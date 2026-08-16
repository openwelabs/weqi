#include "ChessBoard.h"

#include <QPainter>
#include <QMouseEvent>
#include <QPainterPath>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QtMath>
#include <algorithm>

#include "GameController.h"
#include "ChessRules.h"

namespace {

// ---- 现代配色 ----
const QColor kLightSquare(0xEF, 0xE7, 0xD8);   // 浅色格（暖米色）
const QColor kDarkSquare(0xB0, 0x8A, 0x5E);    // 深色格（柔和棕）
const QColor kHoverOverlay(0xFF, 0xFF, 0xFF, 55);
const QColor kSelectedOverlay(0x4C, 0xAF, 0x50, 120);
const QColor kHintDot(0x4C, 0xAF, 0x50, 150);   // 可能移动位置圆点
const QColor kHintDotCapture(0xE0, 0x6C, 0x5A, 160); // 可吃子位置圆环
const QColor kLastMoveOverlay(0xF5, 0xC5, 0x42, 70); // 最近一步高亮
const QColor kCheckOverlay(0xE0, 0x3A, 0x3A, 110);   // 将军高亮
const QColor kBoardBorder(0x6E, 0x54, 0x3A);    // 棋盘外框
const QColor kBoardShadow(0x00, 0x00, 0x00, 90);
const QColor kCoordText(0x8A, 0x7A, 0x66);      // 坐标文字

constexpr int kBoardSize = 8;
constexpr qreal kSquareInset = 0.06;   // 格子内边距（占格宽比例）
constexpr qreal kSquareRadius = 0.14;  // 格子圆角（占格宽比例）
constexpr qreal kBoardRadius = 14.0;   // 棋盘外框圆角（像素）
constexpr qreal kShadowBlur = 24.0;    // 棋盘阴影模糊半径
constexpr qreal kShadowOffset = 6.0;   // 棋盘阴影偏移

// 动画中的棋子：源格、目标格、棋子
struct AnimPiece
{
    int fromRow;
    int fromCol;
    int toRow;
    int toCol;
    ChessPiece piece;
};

} // namespace

ChessBoard::ChessBoard(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true); // 启用无按键时的鼠标移动事件，用于悬停反馈
    setFocusPolicy(Qt::StrongFocus);

    // 落子动画：0..1 进度，映射为从源格到目标格的像素偏移
    m_animation.setDuration(220);
    m_animation.setEasingCurve(QEasingCurve::OutCubic);
    connect(&m_animation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_animOffset = value.toPointF();
        update();
    });
    connect(&m_animation, &QVariantAnimation::finished, this, [this]() {
        m_animating = false;
        // 动画结束后发出信号，并触发落子反馈
        emit moveMade(m_animFromRow, m_animFromCol, m_animToRow, m_animToCol);
        startFeedbackAnimation();
        update();
    });

    // 落子反馈动画：目标格短暂高亮后淡出
    m_feedbackAnim.setDuration(320);
    m_feedbackAnim.setEasingCurve(QEasingCurve::OutCubic);
    connect(&m_feedbackAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_feedbackAlpha = value.toReal();
        update();
    });
}

void ChessBoard::setController(GameController *controller)
{
    m_controller = controller;
    if (m_controller) {
        connect(m_controller, &GameController::moveCommitted,
                this, &ChessBoard::onMoveCommitted);
        connect(m_controller, &GameController::gameStateChanged,
                this, &ChessBoard::onStateChanged);
        connect(m_controller, &GameController::undoPerformed,
                this, &ChessBoard::onUndoPerformed);
        connect(m_controller, &GameController::gameReset,
                this, &ChessBoard::onGameReset);
    }
    update();
}

PieceColor ChessBoard::currentTurn() const
{
    return m_controller ? m_controller->currentTurn() : PieceColor::White;
}

ChessPiece ChessBoard::pieceAt(int row, int col) const
{
    if (!m_controller)
        return ChessPiece();
    return m_controller->state().pieceAt(row, col);
}

QSize ChessBoard::sizeHint() const
{
    return QSize(560, 560);
}

QSize ChessBoard::minimumSizeHint() const
{
    return QSize(240, 240);
}

// 棋盘实际绘制区域：在 widget 内居中，保持正方形
QRectF ChessBoard::boardRect() const
{
    const qreal side = qMin(width(), height());
    const qreal x = (width() - side) / 2.0;
    const qreal y = (height() - side) / 2.0;
    return QRectF(x, y, side, side);
}

QRectF ChessBoard::squareRect(int row, int col) const
{
    const QRectF board = boardRect();
    const qreal cell = board.width() / kBoardSize;
    return QRectF(board.left() + col * cell, board.top() + row * cell, cell, cell);
}

bool ChessBoard::squareAt(const QPointF &pos, int &row, int &col) const
{
    const QRectF board = boardRect();
    if (!board.contains(pos))
        return false;
    const qreal cell = board.width() / kBoardSize;
    col = qFloor((pos.x() - board.left()) / cell);
    row = qFloor((pos.y() - board.top()) / cell);
    return row >= 0 && row < kBoardSize && col >= 0 && col < kBoardSize;
}

void ChessBoard::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF board = boardRect();

    // ---- 棋盘外框阴影 ----
    QPainterPath shadowPath;
    shadowPath.addRoundedRect(board.adjusted(0, kShadowOffset, 0, kShadowOffset), kBoardRadius, kBoardRadius);
    painter.fillPath(shadowPath, kBoardShadow);

    // ---- 棋盘外框（圆角背景）----
    QPainterPath boardPath;
    boardPath.addRoundedRect(board, kBoardRadius, kBoardRadius);
    painter.fillPath(boardPath, kBoardBorder);

    // ---- 绘制 64 个格子 ----
    for (int row = 0; row < kBoardSize; ++row) {
        for (int col = 0; col < kBoardSize; ++col) {
            drawSquare(painter, row, col, squareRect(row, col));
        }
    }

    // 最近一步高亮
    drawLastMoveHighlight(painter);

    // 将军高亮
    drawCheckHighlight(painter);

    // 落子反馈：目标格短暂高亮后淡出
    if (m_feedbackAlpha > 0.0 && m_hasLastMove) {
        const QRectF toRect = squareRect(m_lastToRow, m_lastToCol);
        const qreal inset = toRect.width() * kSquareInset;
        const qreal radius = toRect.width() * kSquareRadius;
        QPainterPath feedbackPath;
        feedbackPath.addRoundedRect(toRect.adjusted(inset, inset, -inset, -inset), radius, radius);
        painter.fillPath(feedbackPath, QColor(0x4C, 0xAF, 0x50, qRound(120 * m_feedbackAlpha)));
    }

    // 可能移动位置提示
    drawMoveHints(painter);

    // ---- 绘制棋子（动画中的棋子单独处理）----
    for (int row = 0; row < kBoardSize; ++row) {
        for (int col = 0; col < kBoardSize; ++col) {
            const ChessPiece &piece = pieceAt(row, col);
            if (!piece.isValid())
                continue;
            // 动画中的棋子由下方单独绘制
            if (m_animating && row == m_animFromRow && col == m_animFromCol)
                continue;
            QRectF rect = squareRect(row, col);
            drawPiece(painter, piece, rect);
        }
    }

    // ---- 绘制动画中的棋子（叠加在目标格上方）----
    if (m_animating) {
        const ChessPiece &piece = pieceAt(m_animFromRow, m_animFromCol);
        QRectF rect = squareRect(m_animToRow, m_animToCol);
        rect.translate(m_animOffset);
        drawPiece(painter, piece, rect);
    }

    // ---- 坐标标注 ----
    drawCoordinates(painter);
}

// 绘制单个棋盘格（圆角 + 内边距 + 状态叠加）
void ChessBoard::drawSquare(QPainter &painter, int row, int col, const QRectF &rect)
{
    const bool isLight = (row + col) % 2 == 0;
    const QColor base = isLight ? kLightSquare : kDarkSquare;

    // 内边距后的圆角矩形
    const qreal inset = rect.width() * kSquareInset;
    const QRectF inner = rect.adjusted(inset, inset, -inset, -inset);
    const qreal radius = rect.width() * kSquareRadius;

    QPainterPath squarePath;
    squarePath.addRoundedRect(inner, radius, radius);

    // 基础填充
    painter.fillPath(squarePath, base);

    // 悬停反馈
    if (row == m_hoverRow && col == m_hoverCol && !m_animating) {
        painter.fillPath(squarePath, kHoverOverlay);
    }
    // 选中高亮
    if (m_hasSelection && row == m_selectedRow && col == m_selectedCol) {
        painter.fillPath(squarePath, kSelectedOverlay);
    }
}

// 最近一步高亮
void ChessBoard::drawLastMoveHighlight(QPainter &painter)
{
    if (!m_hasLastMove)
        return;

    const QRectF fromRect = squareRect(m_lastFromRow, m_lastFromCol);
    const QRectF toRect = squareRect(m_lastToRow, m_lastToCol);
    const qreal inset = fromRect.width() * kSquareInset;
    const qreal radius = fromRect.width() * kSquareRadius;

    QPainterPath fromPath;
    fromPath.addRoundedRect(fromRect.adjusted(inset, inset, -inset, -inset), radius, radius);
    painter.fillPath(fromPath, kLastMoveOverlay);

    QPainterPath toPath;
    toPath.addRoundedRect(toRect.adjusted(inset, inset, -inset, -inset), radius, radius);
    painter.fillPath(toPath, kLastMoveOverlay);
}

// 将军高亮：当前被将军的王格
void ChessBoard::drawCheckHighlight(QPainter &painter)
{
    if (!m_controller || !m_controller->isInCheck())
        return;

    int kingRow, kingCol;
    if (!m_controller->state().findKing(m_controller->currentTurn(), kingRow, kingCol))
        return;

    const QRectF rect = squareRect(kingRow, kingCol);
    const qreal inset = rect.width() * kSquareInset;
    const qreal radius = rect.width() * kSquareRadius;
    QPainterPath path;
    path.addRoundedRect(rect.adjusted(inset, inset, -inset, -inset), radius, radius);
    painter.fillPath(path, kCheckOverlay);
}

// 合法走法提示（来自控制器）
void ChessBoard::drawMoveHints(QPainter &painter)
{
    if (!m_hasSelection)
        return;

    for (const Move &m : m_legalTargets) {
        const int row = m.toRow;
        const int col = m.toCol;
        const QRectF rect = squareRect(row, col);
        const QPointF center = rect.center();
        const qreal cell = rect.width();

        const bool hasPiece = pieceAt(row, col).isValid() || m.isEnPassant;
        if (hasPiece) {
            // 可吃子：绘制圆环
            painter.setPen(QPen(kHintDotCapture, cell * 0.06));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(center, cell * 0.30, cell * 0.30);
        } else {
            // 空位：绘制圆点
            painter.setPen(Qt::NoPen);
            painter.setBrush(kHintDot);
            painter.drawEllipse(center, cell * 0.13, cell * 0.13);
        }
    }
}

// 绘制棋子：渐变填充 + 阴影 + 高光
void ChessBoard::drawPiece(QPainter &painter, const ChessPiece &piece, const QRectF &rect)
{
    const QPainterPath path = piece.path();
    const qreal scale = rect.width();

    painter.save();
    painter.translate(rect.center());
    painter.scale(scale, scale);
    painter.translate(-0.5, -0.5);

    // 棋子阴影（轻微偏移）
    painter.save();
    painter.translate(0.0, 0.03);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 60));
    painter.drawPath(path);
    painter.restore();

    // 主体渐变填充（上亮下暗，营造立体感）
    QLinearGradient grad(0, 0.1, 0, 0.9);
    grad.setColorAt(0.0, piece.highlightColor());
    grad.setColorAt(0.35, piece.fillColor());
    grad.setColorAt(1.0, piece.shadeColor());

    painter.setPen(QPen(piece.strokeColor(), 0.018));
    painter.setBrush(grad);
    painter.drawPath(path);

    painter.restore();
}

// 坐标标注：a-h 列、1-8 行
void ChessBoard::drawCoordinates(QPainter &painter)
{
    const QRectF board = boardRect();
    const qreal cell = board.width() / kBoardSize;

    painter.save();
    painter.setPen(kCoordText);
    QFont font = painter.font();
    font.setPointSizeF(qMax(7.0, cell * 0.16));
    font.setBold(true);
    painter.setFont(font);

    // 列标注（底部）
    for (int col = 0; col < kBoardSize; ++col) {
        const QRectF rect = squareRect(7, col);
        const QChar letter = QChar('a' + col);
        painter.drawText(rect.adjusted(0, 0, 0, -rect.height() * 0.06),
                         Qt::AlignBottom | Qt::AlignHCenter, QString(letter));
    }

    // 行标注（左侧）
    for (int row = 0; row < kBoardSize; ++row) {
        const QRectF rect = squareRect(row, 0);
        const QString num = QString::number(8 - row);
        painter.drawText(rect.adjusted(rect.width() * 0.06, 0, 0, 0),
                         Qt::AlignTop | Qt::AlignLeft, num);
    }

    painter.restore();
}

void ChessBoard::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;
    if (!m_controller || m_controller->result() != GameController::Result::Ongoing)
        return;
    if (m_animating)
        return; // 动画期间限制输入

    int row, col;
    if (!squareAt(event->position(), row, col))
        return;

    const ChessPiece &target = pieceAt(row, col);

    // 已有选中且点击不同格子 -> 尝试移动
    if (m_hasSelection && (row != m_selectedRow || col != m_selectedCol)) {
        const int fromRow = m_selectedRow;
        const int fromCol = m_selectedCol;

        // 点击的是合法目标 -> 请求控制器执行走法
        const bool isLegalTarget = std::any_of(m_legalTargets.begin(), m_legalTargets.end(),
            [row, col](const Move &m) { return m.toRow == row && m.toCol == col; });

        if (isLegalTarget) {
            // 升变走法由控制器处理（会发出 promotionRequired）
            m_controller->tryMove(fromRow, fromCol, row, col);
            return;
        }

        // 点击非合法目标：若点击的是当前回合棋子则改选，否则取消选中
        if (target.isValid() && target.color() == m_controller->currentTurn()) {
            m_hasSelection = true;
            m_selectedRow = row;
            m_selectedCol = col;
            m_legalTargets = m_controller->legalMoves(row, col);
            update();
        } else {
            m_hasSelection = false;
            m_selectedRow = m_selectedCol = -1;
            m_legalTargets.clear();
            update();
        }
        return;
    }

    // 点击有棋子的格子 -> 选中（仅当前回合棋子可选）
    if (target.isValid() && target.color() == m_controller->currentTurn()) {
        m_hasSelection = true;
        m_selectedRow = row;
        m_selectedCol = col;
        m_legalTargets = m_controller->legalMoves(row, col);
        update();
    } else {
        // 点击空格或错误阵营的棋子 -> 取消选中
        m_hasSelection = false;
        m_selectedRow = m_selectedCol = -1;
        m_legalTargets.clear();
        update();
    }
}

void ChessBoard::mouseMoveEvent(QMouseEvent *event)
{
    int row, col;
    if (squareAt(event->position(), row, col)) {
        if (row != m_hoverRow || col != m_hoverCol) {
            m_hoverRow = row;
            m_hoverCol = col;
            update();
        }
    } else if (m_hoverRow != -1 || m_hoverCol != -1) {
        m_hoverRow = m_hoverCol = -1;
        update();
    }
}

void ChessBoard::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    m_hoverRow = m_hoverCol = -1;
    update();
}

void ChessBoard::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    update();
}

void ChessBoard::startMoveAnimation(int fromRow, int fromCol, int toRow, int toCol)
{
    m_animating = true;
    m_animFromRow = fromRow;
    m_animFromCol = fromCol;
    m_animToRow = toRow;
    m_animToCol = toCol;

    // 计算源格到目标格的像素偏移
    const QPointF fromCenter = squareRect(fromRow, fromCol).center();
    const QPointF toCenter = squareRect(toRow, toCol).center();
    const QPointF delta = toCenter - fromCenter;

    m_animation.stop();
    m_animation.setStartValue(QPointF(0, 0));
    m_animation.setEndValue(delta);
    m_animation.start();
}

void ChessBoard::startFeedbackAnimation()
{
    m_feedbackAnim.stop();
    m_feedbackAnim.setStartValue(1.0);
    m_feedbackAnim.setEndValue(0.0);
    m_feedbackAnim.start();
}

// 控制器通知：走子已提交，播放动画
void ChessBoard::onMoveCommitted(const Move &move)
{
    // 清除选中
    m_hasSelection = false;
    m_selectedRow = m_selectedCol = -1;
    m_legalTargets.clear();

    // 记录最近一步（用于高亮）
    m_hasLastMove = true;
    m_lastFromRow = move.fromRow;
    m_lastFromCol = move.fromCol;
    m_lastToRow = move.toRow;
    m_lastToCol = move.toCol;

    // 播放移动动画
    startMoveAnimation(move.fromRow, move.fromCol, move.toRow, move.toCol);
}

void ChessBoard::onStateChanged()
{
    update();
}

void ChessBoard::onUndoPerformed()
{
    m_hasSelection = false;
    m_selectedRow = m_selectedCol = -1;
    m_legalTargets.clear();
    m_animating = false;
    m_animation.stop();
    update();
}

void ChessBoard::onGameReset()
{
    m_hasSelection = false;
    m_selectedRow = m_selectedCol = -1;
    m_legalTargets.clear();
    m_hasLastMove = false;
    m_animating = false;
    m_animation.stop();
    update();
}
