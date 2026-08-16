#include "GameHistoryManager.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

#include "DataPaths.h"

GameHistoryManager::GameHistoryManager(QObject *parent)
    : QObject(parent)
{
    load();
}

const GameRecord *GameHistoryManager::at(int index) const
{
    if (index < 0 || index >= m_records.size())
        return nullptr;
    return &m_records.at(index);
}

void GameHistoryManager::addRecord(const GameRecord &record)
{
    GameRecord rec = record;
    if (rec.id.isEmpty())
        rec.id = QString::number(QDateTime::currentMSecsSinceEpoch());

    // 写入文件
    const QString path = DataPaths::gamesDir() + QStringLiteral("/") + rec.id + QStringLiteral(".json");
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        file.write(QJsonDocument(rec.toJson()).toJson(QJsonDocument::Indented));

    // 插入并保持日期倒序
    m_records.append(rec);
    std::sort(m_records.begin(), m_records.end(),
              [](const GameRecord &a, const GameRecord &b) { return a.date > b.date; });

    emit historyChanged();
}

void GameHistoryManager::removeRecord(const QString &id)
{
    for (int i = 0; i < m_records.size(); ++i) {
        if (m_records.at(i).id == id) {
            QFile::remove(DataPaths::gamesDir() + QStringLiteral("/") + id + QStringLiteral(".json"));
            m_records.removeAt(i);
            emit historyChanged();
            return;
        }
    }
}

void GameHistoryManager::clearAll()
{
    for (const GameRecord &rec : m_records)
        QFile::remove(DataPaths::gamesDir() + QStringLiteral("/") + rec.id + QStringLiteral(".json"));
    m_records.clear();
    emit historyChanged();
}

void GameHistoryManager::load()
{
    m_records.clear();

    const QDir dir(DataPaths::gamesDir());
    const QStringList files = dir.entryList(QStringList() << QStringLiteral("*.json"),
                                            QDir::Files, QDir::Name);
    for (const QString &file : files) {
        QFile f(dir.filePath(file));
        if (!f.open(QIODevice::ReadOnly))
            continue;
        const QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
        m_records.append(GameRecord::fromJson(obj));
    }

    std::sort(m_records.begin(), m_records.end(),
              [](const GameRecord &a, const GameRecord &b) { return a.date > b.date; });
}

QVector<GameRecord> GameHistoryManager::recent(int n) const
{
    QVector<GameRecord> result;
    const int take = qMin(n, m_records.size());
    for (int i = 0; i < take; ++i)
        result.append(m_records.at(i));
    return result;
}
