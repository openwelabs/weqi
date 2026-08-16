#pragma once

#include <QVector>

#include "GameState.h"
#include "Move.h"

// 国际象棋规则引擎。
// 负责：合法走法生成、将军/将死/逼和检测、王车易位、吃过路兵、升变等特殊规则。
// 所有规则由 C++ 实现，不依赖任何外部棋类库。
class ChessRules
{
public:
    // 判断 (row,col) 是否被 byColor 方的棋子攻击
    static bool isSquareAttacked(const GameState &state, int row, int col, PieceColor byColor);

    // 判断 color 方的王是否处于将军状态
    static bool isInCheck(const GameState &state, PieceColor color);

    // 获取 (row,col) 处棋子的所有合法走法（已过滤王安全）
    static QVector<Move> getLegalMoves(const GameState &state, int row, int col);

    // 获取 color 方所有棋子的所有合法走法
    static QVector<Move> getAllLegalMoves(const GameState &state, PieceColor color);

    // 判断 color 方是否被将死
    static bool isCheckmate(const GameState &state, PieceColor color);

    // 判断 color 方是否逼和（无子可动且未被将军）
    static bool isStalemate(const GameState &state, PieceColor color);

    // 判断 color 方是否无合法走法（被将军或逼和）
    static bool hasNoLegalMoves(const GameState &state, PieceColor color);

    // 应用一步棋到 state，返回新状态（不修改原 state）。
    // 处理吃子、易位、吃过路兵、升变、半回合计数、回合切换、易位权限更新。
    static GameState makeMove(const GameState &state, const Move &move);

    // 生成一步棋的代数记谱（如 e4、Nf3、O-O、exd5、e8=Q）
    static QString moveToSan(const GameState &state, const Move &move);

private:
    // 生成伪合法走法（未过滤王安全）
    static QVector<Move> generatePseudoLegalMoves(const GameState &state, int row, int col);

    // 判断一步棋是否合法（不会让自己的王被将军）
    static bool isMoveLegal(const GameState &state, const Move &move);
};
