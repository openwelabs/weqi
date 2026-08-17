#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcess>

#include "AIProviderManager.h"

class QTimer;

// AI 管理器：负责与 Python AI Adapter 通信。
// 通过 QProcess 异步启动 python3 ai_adapter/main.py，向其发送 JSON 请求，
// 读取返回的 JSON 响应，提取 UCI 走法。
//
// 本类只负责 AI 通信与走法提取，不判断走法合法性（由 GameController 负责）。
// 所有请求均为异步，绝不阻塞 UI 主线程。
class AIManager : public QObject
{
    Q_OBJECT

public:
    explicit AIManager(QObject *parent = nullptr);

    // 请求 AI 走法（异步）。返回 true 表示请求已发起。
    // provider: AI Provider 配置
    // fen:      当前局面 FEN
    // turn:     当前回合（"white" / "black"）
    // moveHistory: 已走的 UCI 走法列表
    // legalMoves: 当前方所有合法走法（UCI 格式），AI 必须从中选择一个
    // lastError: 上次 AI 选错的走法反馈（空表示首次请求）。用于自动调教重试，
    //            让 AI 知道上次选错了，重新从合法列表选一个不同的。
    bool requestMove(const AIProvider &provider, const QString &fen,
                     const QString &turn, const QStringList &moveHistory,
                     const QStringList &legalMoves,
                     const QString &lastError = QString());

    // 取消当前请求
    void cancel();

    // 当前是否有请求进行中
    bool isBusy() const { return m_busy; }

    // 设置 Python 解释器路径（默认 "python3"）
    void setPythonPath(const QString &path) { m_pythonPath = path; }

    // 设置 AI Adapter 脚本路径（默认自动探测）
    void setAdapterPath(const QString &path) { m_adapterPath = path; }

    // 设置请求超时（毫秒，默认 60000）
    void setTimeoutMs(int ms) { m_timeoutMs = ms; }

signals:
    // AI 返回了一个 UCI 走法（尚未验证合法性）及一句聊天内容（可能为空）
    void moveReady(const QString &uciMove, const QString &message);

    // 请求失败（网络错误、超时、Python 崩溃、JSON 解析失败等）
    void failed(const QString &error);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onTimeout();

private:
    // 解析 QProcess 输出，提取 UCI 走法
    void parseOutput(const QByteArray &output);

    // 探测 AI Adapter 脚本路径
    QString resolveAdapterPath() const;

    QProcess *m_process = nullptr;
    QTimer *m_timer = nullptr;
    bool m_busy = false;
    QString m_pythonPath = QStringLiteral("python3");
    QString m_adapterPath;
    int m_timeoutMs = 60000;
};
