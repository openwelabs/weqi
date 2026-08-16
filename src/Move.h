#pragma once

#include "ChessPiece.h"

// 一步棋的完整描述。
// 除基本移动外，还携带特殊走法标记（易位、吃过路兵、升变）与吃子信息，
// 供规则引擎、动画与棋谱使用。
struct Move
{
    int fromRow = -1;
    int fromCol = -1;
    int toRow = -1;
    int toCol = -1;

    // 升变目标类型（仅当 isPromotion 为 true 时有效）
    PieceType promotion = PieceType::None;

    // 特殊走法标记
    bool isCastleKingside = false;   // O-O
    bool isCastleQueenside = false;  // O-O-O
    bool isEnPassant = false;        // 吃过路兵
    bool isPromotion = false;        // 升变

    // 被吃掉的棋子（吃过路兵时是过路兵所在格）
    PieceType captured = PieceType::None;
    int capturedRow = -1;
    int capturedCol = -1;

    // 移动的棋子
    PieceType piece = PieceType::None;
    PieceColor color = PieceColor::White;

    bool isValid() const { return fromRow >= 0 && toRow >= 0; }
};
