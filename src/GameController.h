#pragma once

#include <QObject>
#include <QVector>

#include "GameState.h"
#include "Move.h"
#include "MoveHistory.h"

// 棋局控制器：连接 UI 与规则引擎，管理棋局流程。
// 持有当前 GameState 与 MoveHistory，负责走子、悔棋、新游戏、
// 将军/将死/逼和/和棋检测，并通过信号通知 UI 更新。
class GameController : public QObject
{
    Q_OBJECT

public:
    // 棋局结果
    enum class Result {
        Ongoing,     // 进行中
        WhiteWin,    // 白方胜（将死）
        BlackWin,    // 黑方胜（将死）
        Draw         // 和棋
    };

    explicit GameController(QObject *parent = nullptr);

    // 当前状态
    const GameState &state() const { return m_state; }
    const MoveHistory &history() const { return m_history; }
    PieceColor currentTurn() const { return m_state.turn(); }
    Result result() const { return m_result; }
    QString resultReason() const { return m_resultReason; }

    // 当前是否处于将军状态
    bool isInCheck() const;

    // 获取 (row,col) 处棋子的合法走法
    QVector<Move> legalMoves(int row, int col) const;

    // 尝试走子。若为升变走法，返回 false 并发出 promotionRequired 信号，
    // 等待 UI 选择升变目标后调用 completePromotion。
    // 成功执行返回 true。
    bool tryMove(int fromRow, int fromCol, int toRow, int toCol);

    // 完成升变（promotion 为后/车/象/马）
    void completePromotion(PieceType promotion);

    // 悔棋：撤销最后一步，返回是否成功
    bool undo();

    // 新游戏
    void newGame();

    // 从 FEN 恢复局面（用于 Continue Game）；解析失败返回 false
    bool loadFen(const QString &fen);

    // 当前将军/将死/逼和状态描述（用于状态栏）
    QString statusText() const;

signals:
    // 走子完成（动画结束后由 UI 触发确认），携带走法信息
    void moveCommitted(const Move &move);

    // 需要升变选择（UI 应弹出升变面板）
    void promotionRequired(int fromRow, int fromCol, int toRow, int toCol);

    // 棋局状态变化（将军/将死/和棋等）
    void gameStateChanged();

    // 棋局结束
    void gameOver(Result result, const QString &reason);

    // 悔棋完成
    void undoPerformed();

    // 新游戏开始
    void gameReset();

private:
    // 执行一步棋（内部），更新状态与历史，检测结果
    void applyMove(const Move &move);

    // 检测并设置棋局结果（将死/逼和/五十回合/三次重复/无子可胜）
    void evaluateResult();

    // 判断当前局面是否已出现三次（用于重复检测）
    bool isThreefoldRepetition() const;

    // 判断是否无子可胜（王对王、王+象/马对王等）
    bool isInsufficientMaterial() const;

    GameState m_state;
    MoveHistory m_history;

    Result m_result = Result::Ongoing;
    QString m_resultReason;

    // 待完成的升变走法
    Move m_pendingPromotion;
    bool m_hasPendingPromotion = false;

    // 历史局面 FEN 计数（用于三次重复检测）
    QVector<QString> m_positionHistory;
};
