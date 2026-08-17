#include "StatsManager.h"

#include "GameHistoryManager.h"

StatsManager::StatsManager(QObject *parent)
    : QObject(parent)
{
}

void StatsManager::setHistory(GameHistoryManager *history)
{
    m_history = history;
    if (m_history) {
        connect(m_history, &GameHistoryManager::historyChanged,
                this, &StatsManager::refresh);
    }
    refresh();
}

void StatsManager::refresh()
{
    PlayerStats s;
    if (!m_history) {
        m_stats = s;
        emit statsChanged();
        return;
    }

    const QVector<GameRecord> &records = m_history->records();
    s.gamesPlayed = 0;

    // 当前连胜/连败：从最新记录往前数，直到结果变化
    int streak = 0;
    int bestWin = 0;
    int bestLoss = 0;
    int curWin = 0;
    int curLoss = 0;

    for (int i = 0; i < records.size(); ++i) {
        const GameRecord &rec = records.at(i);
        // AI vs AI 对局不计入玩家战绩（仅作历史记录）
        if (rec.mode == GameMode::AIVsAI)
            continue;
        ++s.gamesPlayed;
        const GameOutcome o = rec.outcome;
        if (o == GameOutcome::Win) {
            ++s.wins;
            ++curWin;
            curLoss = 0;
            if (curWin > bestWin)
                bestWin = curWin;
        } else if (o == GameOutcome::Loss) {
            ++s.losses;
            ++curLoss;
            curWin = 0;
            if (curLoss > bestLoss)
                bestLoss = curLoss;
        } else if (o == GameOutcome::Draw) {
            ++s.draws;
            curWin = 0;
            curLoss = 0;
        }
    }

    s.bestWinStreak = bestWin;
    s.bestLossStreak = bestLoss;

    // 当前连胜/连败：从最新记录开始（跳过 AI vs AI 记录）
    for (int i = 0; i < records.size(); ++i) {
        if (records.at(i).mode == GameMode::AIVsAI)
            continue;
        const GameOutcome latest = records.at(i).outcome;
        if (latest == GameOutcome::Win || latest == GameOutcome::Loss) {
            int count = 0;
            for (int j = i; j < records.size(); ++j) {
                if (records.at(j).mode == GameMode::AIVsAI)
                    continue;
                if (records.at(j).outcome == latest)
                    ++count;
                else
                    break;
            }
            streak = (latest == GameOutcome::Win) ? count : -count;
        }
        break;
    }
    s.currentStreak = streak;

    // 胜率
    if (s.gamesPlayed > 0)
        s.winRate = (static_cast<double>(s.wins) / s.gamesPlayed) * 100.0;

    m_stats = s;
    emit statsChanged();
}
