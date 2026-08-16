#pragma once

#include <QVector>
#include <QString>

#include "ChessPiece.h"

// 棋局状态：棋子位置、当前回合、王车易位权限、吃过路兵目标格、
// 半回合计数（五十回合规则）与重复局面信息。
// 这是规则引擎操作的核心数据模型，与 UI 完全解耦。
class GameState
{
public:
    static constexpr int kBoardSize = 8;

    GameState();

    // 重置为初始局面
    void reset();

    // 棋子访问
    ChessPiece pieceAt(int row, int col) const;
    void setPiece(int row, int col, const ChessPiece &piece);

    // 当前回合
    PieceColor turn() const { return m_turn; }
    void setTurn(PieceColor turn) { m_turn = turn; }

    // 王车易位权限
    bool canCastleKingside(PieceColor color) const;
    bool canCastleQueenside(PieceColor color) const;
    void setCastlingRights(PieceColor color, bool kingside, bool queenside);

    // 吃过路兵目标格（-1 表示无）
    int enPassantRow() const { return m_enPassantRow; }
    int enPassantCol() const { return m_enPassantCol; }
    void setEnPassant(int row, int col);
    void clearEnPassant();

    // 半回合计数（自上次吃子或兵移动以来的步数）
    int halfmoveClock() const { return m_halfmoveClock; }
    void setHalfmoveClock(int n) { m_halfmoveClock = n; }

    // 完整回合数（从 1 开始，黑方走完后递增）
    int fullmoveNumber() const { return m_fullmoveNumber; }
    void setFullmoveNumber(int n) { m_fullmoveNumber = n; }

    // 查找某方王的位置；未找到返回 false
    bool findKing(PieceColor color, int &row, int &col) const;

    // 生成 FEN 字符串（用于重复局面检测与棋谱）
    QString toFen() const;

    // 从 FEN 字符串恢复局面；解析失败返回 false 并保持原状态
    bool fromFen(const QString &fen);

    // 深拷贝
    GameState copy() const;

private:
    QVector<ChessPiece> m_squares; // 索引 = row * 8 + col

    PieceColor m_turn = PieceColor::White;

    // 易位权限：whiteKingside, whiteQueenside, blackKingside, blackQueenside
    bool m_castleWK = true;
    bool m_castleWQ = true;
    bool m_castleBK = true;
    bool m_castleBQ = true;

    // 吃过路兵目标格（-1 表示无）
    int m_enPassantRow = -1;
    int m_enPassantCol = -1;

    int m_halfmoveClock = 0;
    int m_fullmoveNumber = 1;
};
