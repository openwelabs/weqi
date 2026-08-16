#include "ChessPiece.h"

#include <QPainterPath>

ChessPiece::ChessPiece(PieceType type, PieceColor color)
    : m_type(type)
    , m_color(color)
{
}

QColor ChessPiece::fillColor() const
{
    // 白棋用暖白，黑棋用深灰蓝，都带一点现代感
    return m_color == PieceColor::White ? QColor(0xFB, 0xF9, 0xF4) : QColor(0x3A, 0x3D, 0x55);
}

QColor ChessPiece::shadeColor() const
{
    // 渐变暗部：比主体略深
    return m_color == PieceColor::White ? QColor(0xD9, 0xD2, 0xC4) : QColor(0x1E, 0x20, 0x33);
}

QColor ChessPiece::strokeColor() const
{
    return m_color == PieceColor::White ? QColor(0xB8, 0xB0, 0xA0) : QColor(0x14, 0x15, 0x24);
}

QColor ChessPiece::highlightColor() const
{
    // 顶部受光高光
    return m_color == PieceColor::White ? QColor(0xFF, 0xFF, 0xFF) : QColor(0x5A, 0x5E, 0x7A);
}

// 所有棋子路径都绘制在单位正方形 (0,0)-(1,1) 内，绘制时再缩放。
// 坐标以棋盘格中心为基准，棋子主体大致占据 0.12..0.88 的纵向范围。
QPainterPath ChessPiece::path() const
{
    QPainterPath p;
    switch (m_type) {
    case PieceType::Pawn:
        // 兵：圆头 + 细颈 + 底座
        p.addEllipse(QRectF(0.34, 0.16, 0.32, 0.30));
        p.moveTo(0.40, 0.46);
        p.lineTo(0.60, 0.46);
        p.lineTo(0.60, 0.56);
        p.lineTo(0.40, 0.56);
        p.closeSubpath();
        p.moveTo(0.30, 0.56);
        p.lineTo(0.70, 0.56);
        p.lineTo(0.74, 0.72);
        p.lineTo(0.26, 0.72);
        p.closeSubpath();
        p.moveTo(0.22, 0.72);
        p.lineTo(0.78, 0.72);
        p.lineTo(0.82, 0.84);
        p.lineTo(0.18, 0.84);
        p.closeSubpath();
        break;

    case PieceType::Knight:
        // 马：头部 + 颈部 + 底座
        p.moveTo(0.30, 0.84);
        p.lineTo(0.30, 0.62);
        p.cubicTo(0.30, 0.50, 0.36, 0.42, 0.46, 0.38);
        p.cubicTo(0.52, 0.36, 0.56, 0.30, 0.56, 0.24);
        p.cubicTo(0.56, 0.18, 0.62, 0.16, 0.66, 0.20);
        p.cubicTo(0.70, 0.24, 0.68, 0.30, 0.64, 0.34);
        p.cubicTo(0.62, 0.38, 0.64, 0.42, 0.68, 0.44);
        p.cubicTo(0.74, 0.48, 0.76, 0.54, 0.74, 0.60);
        p.cubicTo(0.72, 0.66, 0.66, 0.70, 0.60, 0.70);
        p.lineTo(0.60, 0.84);
        p.closeSubpath();
        p.moveTo(0.22, 0.84);
        p.lineTo(0.78, 0.84);
        p.lineTo(0.82, 0.92);
        p.lineTo(0.18, 0.92);
        p.closeSubpath();
        break;

    case PieceType::Bishop:
        // 象：圆顶 + 斜切身体 + 底座
        p.addEllipse(QRectF(0.36, 0.12, 0.28, 0.26));
        p.moveTo(0.40, 0.38);
        p.lineTo(0.60, 0.38);
        p.lineTo(0.66, 0.52);
        p.lineTo(0.58, 0.52);
        p.lineTo(0.62, 0.66);
        p.lineTo(0.38, 0.66);
        p.lineTo(0.42, 0.52);
        p.lineTo(0.34, 0.52);
        p.closeSubpath();
        p.moveTo(0.26, 0.66);
        p.lineTo(0.74, 0.66);
        p.lineTo(0.78, 0.78);
        p.lineTo(0.22, 0.78);
        p.closeSubpath();
        p.moveTo(0.20, 0.78);
        p.lineTo(0.80, 0.78);
        p.lineTo(0.84, 0.90);
        p.lineTo(0.16, 0.90);
        p.closeSubpath();
        break;

    case PieceType::Rook:
        // 车：城垛顶 + 直身 + 底座
        p.moveTo(0.30, 0.20);
        p.lineTo(0.30, 0.12);
        p.lineTo(0.40, 0.12);
        p.lineTo(0.40, 0.20);
        p.lineTo(0.60, 0.20);
        p.lineTo(0.60, 0.12);
        p.lineTo(0.70, 0.12);
        p.lineTo(0.70, 0.20);
        p.lineTo(0.74, 0.20);
        p.lineTo(0.74, 0.40);
        p.lineTo(0.70, 0.40);
        p.lineTo(0.70, 0.70);
        p.lineTo(0.30, 0.70);
        p.lineTo(0.30, 0.40);
        p.lineTo(0.26, 0.40);
        p.lineTo(0.26, 0.20);
        p.closeSubpath();
        p.moveTo(0.22, 0.70);
        p.lineTo(0.78, 0.70);
        p.lineTo(0.82, 0.82);
        p.lineTo(0.18, 0.82);
        p.closeSubpath();
        p.moveTo(0.18, 0.82);
        p.lineTo(0.82, 0.82);
        p.lineTo(0.86, 0.92);
        p.lineTo(0.14, 0.92);
        p.closeSubpath();
        break;

    case PieceType::Queen:
        // 后：皇冠 + 圆点 + 身体
        p.addEllipse(QRectF(0.30, 0.10, 0.12, 0.12));
        p.addEllipse(QRectF(0.44, 0.06, 0.12, 0.12));
        p.addEllipse(QRectF(0.58, 0.10, 0.12, 0.12));
        p.moveTo(0.30, 0.22);
        p.lineTo(0.26, 0.40);
        p.lineTo(0.36, 0.34);
        p.lineTo(0.50, 0.44);
        p.lineTo(0.64, 0.34);
        p.lineTo(0.74, 0.40);
        p.lineTo(0.70, 0.22);
        p.lineTo(0.62, 0.30);
        p.lineTo(0.50, 0.20);
        p.lineTo(0.38, 0.30);
        p.closeSubpath();
        p.moveTo(0.30, 0.44);
        p.lineTo(0.70, 0.44);
        p.lineTo(0.74, 0.60);
        p.lineTo(0.26, 0.60);
        p.closeSubpath();
        p.moveTo(0.24, 0.60);
        p.lineTo(0.76, 0.60);
        p.lineTo(0.80, 0.72);
        p.lineTo(0.20, 0.72);
        p.closeSubpath();
        p.moveTo(0.18, 0.72);
        p.lineTo(0.82, 0.72);
        p.lineTo(0.86, 0.84);
        p.lineTo(0.14, 0.84);
        p.closeSubpath();
        break;

    case PieceType::King:
        // 王：十字顶 + 圆头 + 身体
        p.moveTo(0.46, 0.06);
        p.lineTo(0.54, 0.06);
        p.lineTo(0.54, 0.14);
        p.lineTo(0.62, 0.14);
        p.lineTo(0.62, 0.22);
        p.lineTo(0.54, 0.22);
        p.lineTo(0.54, 0.30);
        p.lineTo(0.46, 0.30);
        p.lineTo(0.46, 0.22);
        p.lineTo(0.38, 0.22);
        p.lineTo(0.38, 0.14);
        p.lineTo(0.46, 0.14);
        p.closeSubpath();
        p.addEllipse(QRectF(0.34, 0.30, 0.32, 0.26));
        p.moveTo(0.30, 0.56);
        p.lineTo(0.70, 0.56);
        p.lineTo(0.74, 0.68);
        p.lineTo(0.26, 0.68);
        p.closeSubpath();
        p.moveTo(0.24, 0.68);
        p.lineTo(0.76, 0.68);
        p.lineTo(0.80, 0.80);
        p.lineTo(0.20, 0.80);
        p.closeSubpath();
        p.moveTo(0.18, 0.80);
        p.lineTo(0.82, 0.80);
        p.lineTo(0.86, 0.92);
        p.lineTo(0.14, 0.92);
        p.closeSubpath();
        break;

    case PieceType::None:
    default:
        break;
    }
    return p;
}
