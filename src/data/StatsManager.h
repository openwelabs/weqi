#pragma once

#include <QObject>

#include "GameRecord.h"

class GameHistoryManager;

// 战绩统计：从 GameHistoryManager 的历史记录动态计算。
// 不维护独立战绩数据，避免与历史记录不一致。
struct PlayerStats
{
    int gamesPlayed = 0;
    int wins = 0;
    int losses = 0;
    int draws = 0;
    int currentStreak = 0;   // 当前连胜/连败（正=胜，负=负）
    int bestWinStreak = 0;   // 最高连胜
    int bestLossStreak = 0;  // 最高连败
    double winRate = 0.0;    // 胜率（0..100）
};

// 战绩统计管理器：根据历史对局计算玩家战绩。
class StatsManager : public QObject
{
    Q_OBJECT

public:
    explicit StatsManager(QObject *parent = nullptr);

    // 绑定历史管理器（用于动态计算）
    void setHistory(GameHistoryManager *history);

    // 重新计算统计
    void refresh();

    const PlayerStats &stats() const { return m_stats; }

    // 便捷访问
    int gamesPlayed() const { return m_stats.gamesPlayed; }
    int wins() const { return m_stats.wins; }
    int losses() const { return m_stats.losses; }
    int draws() const { return m_stats.draws; }
    int currentStreak() const { return m_stats.currentStreak; }
    int bestWinStreak() const { return m_stats.bestWinStreak; }
    double winRate() const { return m_stats.winRate; }

signals:
    void statsChanged();

private:
    GameHistoryManager *m_history = nullptr;
    PlayerStats m_stats;
};
