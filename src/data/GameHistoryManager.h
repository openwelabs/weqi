#pragma once

#include <QObject>
#include <QVector>

#include "GameRecord.h"

// 历史对局管理：保存、加载、查询历史对局记录。
// 每盘棋一个 JSON 文件，存放在 games/ 目录。
class GameHistoryManager : public QObject
{
    Q_OBJECT

public:
    explicit GameHistoryManager(QObject *parent = nullptr);

    const QVector<GameRecord> &records() const { return m_records; }
    int count() const { return m_records.size(); }
    const GameRecord *at(int index) const;

    // 添加一条对局记录并持久化
    void addRecord(const GameRecord &record);

    // 删除一条记录
    void removeRecord(const QString &id);

    // 清空所有记录
    void clearAll();

    // 重新加载所有记录（按日期倒序）
    void load();

    // 最近 N 条记录（按日期倒序）
    QVector<GameRecord> recent(int n) const;

signals:
    void historyChanged();

private:
    QVector<GameRecord> m_records;
};
