#pragma once

#include <QColor>
#include <QPainterPath>

// 棋子类型
enum class PieceType {
    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King,
    None
};

// 棋子颜色
enum class PieceColor {
    White,
    Black
};

// 棋子数据模型：类型 + 颜色，并提供矢量绘制路径
class ChessPiece
{
public:
    ChessPiece() = default;
    ChessPiece(PieceType type, PieceColor color);

    bool isValid() const { return m_type != PieceType::None; }
    PieceType type() const { return m_type; }
    PieceColor color() const { return m_color; }

    // 返回绘制在单位正方形（0..1）内的棋子矢量路径
    QPainterPath path() const;

    // 返回棋子主体填充颜色（用于渐变的高光端）
    QColor fillColor() const;

    // 返回棋子主体渐变暗部颜色
    QColor shadeColor() const;

    // 返回棋子描边颜色
    QColor strokeColor() const;

    // 返回棋子高光（顶部受光）颜色
    QColor highlightColor() const;

private:
    PieceType m_type = PieceType::None;
    PieceColor m_color = PieceColor::White;
};
