#pragma once

#include <QWidget>
#include <QVector>
#include <QVariantAnimation>
#include <QPoint>
#include <QPainter>
#include <QSet>

#include "ChessPiece.h"
#include "Move.h"

class GameController;

// 棋盘：负责绘制、鼠标交互与落子动画。
// 棋局状态与规则由 GameController / ChessRules 维护，本类只负责 UI 呈现与交互。
class ChessBoard : public QWidget
{
    Q_OBJECT

public:
    explicit ChessBoard(QWidget *parent = nullptr);

    // 绑定棋局控制器（在显示前调用）
    void setController(GameController *controller);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    // 当前回合（白方先手）
    PieceColor currentTurn() const;

signals:
    // 落子完成（动画结束后发出），供主窗口更新信息面板
    void moveMade(int fromRow, int fromCol, int toRow, int toCol);

    // 需要升变选择（转发给主窗口弹出面板）
    void promotionRequested(int fromRow, int fromCol, int toRow, int toCol);

public slots:
    // 由控制器通知：走子已提交，播放动画
    void onMoveCommitted(const Move &move);

    // 由控制器通知：棋局状态变化（将军/将死/和棋），刷新绘制
    void onStateChanged();

    // 由控制器通知：悔棋完成，刷新棋盘
    void onUndoPerformed();

    // 由控制器通知：新游戏开始
    void onGameReset();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    // 棋盘格坐标 -> 像素矩形
    QRectF squareRect(int row, int col) const;
    // 像素坐标 -> 棋盘格坐标；越界返回 false
    bool squareAt(const QPointF &pos, int &row, int &col) const;
    // 棋盘实际绘制区域（保持正方形）
    QRectF boardRect() const;

    void startMoveAnimation(int fromRow, int fromCol, int toRow, int toCol);
    void startFeedbackAnimation();
    void drawPiece(QPainter &painter, const ChessPiece &piece, const QRectF &rect);
    void drawSquare(QPainter &painter, int row, int col, const QRectF &rect);
    void drawCoordinates(QPainter &painter);
    void drawMoveHints(QPainter &painter);
    void drawLastMoveHighlight(QPainter &painter);
    void drawCheckHighlight(QPainter &painter);

    // 从控制器读取棋子
    ChessPiece pieceAt(int row, int col) const;

    GameController *m_controller = nullptr;

    // 选中状态
    bool m_hasSelection = false;
    int m_selectedRow = -1;
    int m_selectedCol = -1;

    // 合法走法提示（来自控制器）
    QVector<Move> m_legalTargets;

    // 悬停状态
    int m_hoverRow = -1;
    int m_hoverCol = -1;

    // 最近一步（用于高亮）
    bool m_hasLastMove = false;
    int m_lastFromRow = -1;
    int m_lastFromCol = -1;
    int m_lastToRow = -1;
    int m_lastToCol = -1;

    // 落子反馈动画（目标格短暂高亮）
    QVariantAnimation m_feedbackAnim;
    qreal m_feedbackAlpha = 0.0;

    // 移动动画状态
    QVariantAnimation m_animation;
    bool m_animating = false;
    int m_animFromRow = -1;
    int m_animFromCol = -1;
    int m_animToRow = -1;
    int m_animToCol = -1;
    QPointF m_animOffset; // 当前动画偏移（像素）
};
