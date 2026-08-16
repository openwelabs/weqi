#pragma once

#include <QString>
#include <QDateTime>
#include <QJsonObject>

// 对局类型
enum class GameMode {
    HumanVsHuman,  // 真人对战
    HumanVsAI,     // 人机对战
    AIVsAI,        // AI 对战
    Replay         // 复盘
};

// 对局结果（从玩家视角）
enum class GameOutcome {
    Win,     // 胜
    Loss,    // 负
    Draw,    // 和
    Ongoing  // 未完成
};

// 一条历史对局记录。
// 由 GameHistoryManager 持久化到系统用户数据目录。
struct GameRecord
{
    QString id;              // 唯一标识（时间戳）
    QDateTime date;          // 对局时间
    GameMode mode;           // 对局类型
    QString opponent;        // 对手名称（AI 名或 "Human"）
    QString whiteName;       // 白方名称
    QString blackName;       // 黑方名称
    GameOutcome outcome;     // 玩家视角结果
    QString resultText;      // 结果文本（如 "1-0"、"½-½"）
    QString reason;          // 结束原因（将死/逼和等）
    int ratingChange = 0;    // Rating 变化
    int moveCount = 0;       // 步数
    QString pgn;             // 完整棋谱（PGN）

    // 序列化
    QJsonObject toJson() const;
    static GameRecord fromJson(const QJsonObject &obj);
};
