#pragma once

// 简单的 Elo Rating 计算工具。
// 用于本地个人 Rating 调整，不做在线排行榜。
class RatingSystem
{
public:
    // 计算期望得分（0..1），playerRating 对 opponentRating
    static double expectedScore(int playerRating, int opponentRating);

    // 计算 Rating 变化量。
    // playerRating: 玩家当前 Rating
    // opponentRating: 对手 Rating
    // result: 1.0 胜 / 0.5 和 / 0.0 负
    // kFactor: 调整系数（默认 32）
    static int ratingDelta(int playerRating, int opponentRating, double result, int kFactor = 32);
};
