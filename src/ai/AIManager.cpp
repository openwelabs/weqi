#include "AIManager.h"

#include <QProcess>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDir>

namespace {

// 从 JSON 响应中提取 UCI 走法。
// 成功：{"ok": true, "move": "e2e4"}
// 失败：{"ok": false, "error": "..."}
bool parseMoveResponse(const QByteArray &data, QString &move, QString &error)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        error = QStringLiteral("JSON 解析失败");
        return false;
    }

    const QJsonObject obj = doc.object();
    if (!obj.value(QStringLiteral("ok")).toBool()) {
        error = obj.value(QStringLiteral("error")).toString(QStringLiteral("AI 请求失败"));
        return false;
    }

    move = obj.value(QStringLiteral("move")).toString();
    if (move.isEmpty()) {
        error = QStringLiteral("AI 未返回走法");
        return false;
    }
    return true;
}

} // namespace

AIManager::AIManager(QObject *parent)
    : QObject(parent)
{
    m_process = new QProcess(this);
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);

    connect(m_process, &QProcess::finished,
            this, &AIManager::onProcessFinished);
    connect(m_timer, &QTimer::timeout,
            this, &AIManager::onTimeout);
}

bool AIManager::requestMove(const AIProvider &provider, const QString &fen,
                            const QString &turn, const QStringList &moveHistory,
                            const QStringList &legalMoves)
{
    if (m_busy)
        return false;

    // 构造请求 JSON
    QJsonObject providerObj;
    providerObj[QStringLiteral("base_url")] = provider.baseUrl;
    providerObj[QStringLiteral("api_key")] = provider.apiKey;
    providerObj[QStringLiteral("model")] = provider.model;

    QJsonObject gameObj;
    gameObj[QStringLiteral("fen")] = fen;
    gameObj[QStringLiteral("turn")] = turn;

    QJsonArray historyArr;
    for (const QString &m : moveHistory)
        historyArr.append(m);
    gameObj[QStringLiteral("move_history")] = historyArr;

    QJsonArray legalArr;
    for (const QString &m : legalMoves)
        legalArr.append(m);
    gameObj[QStringLiteral("legal_moves")] = legalArr;

    QJsonObject request;
    request[QStringLiteral("action")] = QStringLiteral("get_ai_move");
    request[QStringLiteral("provider")] = providerObj;
    request[QStringLiteral("game")] = gameObj;

    const QByteArray payload = QJsonDocument(request).toJson(QJsonDocument::Compact);

    // 启动 Python 子进程
    const QString adapterPath = resolveAdapterPath();
    if (adapterPath.isEmpty()) {
        emit failed(QStringLiteral("找不到 AI Adapter 脚本"));
        return false;
    }

    m_process->start(m_pythonPath, QStringList() << adapterPath);
    if (!m_process->waitForStarted(3000)) {
        emit failed(QStringLiteral("无法启动 Python（请确认已安装 python3）"));
        return false;
    }

    m_busy = true;
    m_process->write(payload);
    m_process->closeWriteChannel();

    // 启动超时计时器
    m_timer->start(m_timeoutMs);
    return true;
}

void AIManager::cancel()
{
    if (!m_busy)
        return;
    m_timer->stop();
    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(1000);
    }
    m_busy = false;
}

void AIManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode);
    if (!m_busy)
        return;
    m_timer->stop();
    m_busy = false;

    const QByteArray output = m_process->readAllStandardOutput();

    // 进程异常退出（崩溃 / 被 kill）
    if (exitStatus != QProcess::NormalExit) {
        emit failed(QStringLiteral("AI 进程异常退出"));
        return;
    }

    // 解析输出
    QString move;
    QString error;
    if (parseMoveResponse(output, move, error)) {
        emit moveReady(move);
    } else {
        emit failed(error);
    }
}

void AIManager::onTimeout()
{
    if (!m_busy)
        return;
    m_busy = false;
    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(1000);
    }
    emit failed(QStringLiteral("AI 请求超时"));
}

QString AIManager::resolveAdapterPath() const
{
    if (!m_adapterPath.isEmpty())
        return m_adapterPath;

    // 优先使用可执行文件所在目录下的 ai_adapter/main.py
    const QString exeDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        exeDir + QStringLiteral("/ai_adapter/main.py"),
        exeDir + QStringLiteral("/../ai_adapter/main.py"),
        exeDir + QStringLiteral("/../../ai_adapter/main.py"),
    };
    for (const QString &path : candidates) {
        if (QFileInfo::exists(path))
            return QFileInfo(path).absoluteFilePath();
    }
    return QString();
}
