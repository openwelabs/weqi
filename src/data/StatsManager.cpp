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
    s.gamesPlayed = records.size();

    // 当前连胜/连败：从最新记录往前数，直到结果变化
    int streak = 0;
    int bestWin = 0;
    int bestLoss = 0;
    int curWin = 0;
    int curLoss = 0;

    for (int i = 0; i < records.size(); ++i) {
        const GameOutcome o = records.at(i).outcome;
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

    // 当前连胜/连败：从最新记录开始
    if (!records.isEmpty()) {
        const GameOutcome latest = records.first().outcome;
        if (latest == GameOutcome::Win || latest == GameOutcome::Loss) {
            int count = 0;
            for (int i = 0; i < records.size(); ++i) {
                if (records.at(i).outcome == latest)
                    ++count;
                else
                    break;
            }
            streak = (latest == GameOutcome::Win) ? count : -count;
        }
    }
    s.currentStreak = streak;

    // 胜率
    if (s.gamesPlayed > 0)
        s.winRate = (static_cast<double>(s.wins) / s.gamesPlayed) * 100.0;

    m_stats = s;
    emit statsChanged();
}
