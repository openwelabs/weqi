#pragma once

#include <QObject>
#include <QVector>
#include <QString>

#include "GameState.h"
#include "Move.h"
#include "MoveHistory.h"
#include "AIProviderManager.h"

class AIManager;

// 棋局控制器：连接 UI 与规则引擎，管理棋局流程。
// 持有当前 GameState 与 MoveHistory，负责走子、悔棋、新游戏、
// 将军/将死/逼和/和棋检测，并通过信号通知 UI 更新。
//
// AI 集成：当某方为 AI 时，走子后自动触发 AI 请求（异步），
// 收到 AI 走法后由 C++ 验证合法性，非法则重试（最多 3 次）。
// AI 只提出走法，不直接修改棋盘状态。
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

    // ---- AI 集成 ----

    // 设置 AI 管理器（由 MainWindow 注入）
    void setAIManager(AIManager *manager);

    // 设置 AI 对局配置。
    // whiteIsAI / blackIsAI: 该方是否为 AI
    // whiteProvider / blackProvider: 对应 AI 的 Provider（仅当该方为 AI 时有效）
    void setAIGame(bool whiteIsAI, bool blackIsAI,
                   const AIProvider &whiteProvider, const AIProvider &blackProvider);

    // 当前是否处于 AI 思考状态
    bool isAIThinking() const { return m_aiThinking; }

    // 当前思考的 AI 名称（用于 UI 显示）
    QString aiThinkingName() const { return m_aiThinkingName; }

    // 当前思考的 AI 模型（用于 UI 显示）
    QString aiThinkingModel() const { return m_aiThinkingModel; }

    // 取消当前 AI 请求
    void cancelAI();

    // 重试当前 AI 请求（用于 AI 请求失败后的重试）
    void retryAI();

    // 当前回合是否为 AI
    bool isCurrentTurnAI() const;

    // 白方是否为 AI
    bool isWhiteAI() const { return m_whiteIsAI; }

    // 黑方是否为 AI
    bool isBlackAI() const { return m_blackIsAI; }

    // 白方 AI Provider 名称（若白方为 AI）
    QString whiteAIProviderName() const { return m_whiteProvider.name; }

    // 白方 AI 模型（若白方为 AI）
    QString whiteAIModel() const { return m_whiteProvider.model; }

    // 黑方 AI Provider 名称（若黑方为 AI）
    QString blackAIProviderName() const { return m_blackProvider.name; }

    // 黑方 AI 模型（若黑方为 AI）
    QString blackAIModel() const { return m_blackProvider.model; }

    // 将 AI 返回的短代数记法（如 "e4"、"Nf3"、"exd5"、"O-O"）解析为合法走法。
    // 通过把所有合法走法转成 SAN 后与输入匹配实现。找不到返回无效 Move。
    Move resolveMoveFromSan(const QString &san) const;

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

    // AI 思考状态变化（thinking=true 表示 AI 正在思考）
    void aiThinkingChanged(bool thinking, const QString &name, const QString &model);

    // AI 请求失败（UI 应显示错误并提供重试/取消）
    void aiRequestFailed(const QString &error);

private:
    // 执行一步棋（内部），更新状态与历史，检测结果
    void applyMove(const Move &move);

    // 检测并设置棋局结果（将死/逼和/五十回合/三次重复/无子可胜）
    void evaluateResult();

    // 判断当前局面是否已出现三次（用于重复检测）
    bool isThreefoldRepetition() const;

    // 判断是否无子可胜（王对王、王+象/马对王等）
    bool isInsufficientMaterial() const;

    // ---- AI 内部 ----

    // 若当前回合为 AI，触发 AI 请求
    void maybeTriggerAI();

    // 处理 AI 返回的走法（验证合法性，非法则重试）
    void onAIMoveReady(const QString &uciMove);

    // 处理 AI 请求失败
    void onAIFailed(const QString &error);

    // 将 UCI 走法（如 "e2e4"）转换为 Move 结构
    Move uciToMove(const QString &uci) const;

    // 将 Move 结构转换为 UCI 走法字符串（如 "e2e4"、"e7e8q"）
    QString moveToUci(const Move &move) const;

    // 当前回合对应的 AI Provider（若当前回合为 AI）
    const AIProvider *currentAIProvider() const;

    GameState m_state;
    MoveHistory m_history;

    Result m_result = Result::Ongoing;
    QString m_resultReason;

    // 待完成的升变走法
    Move m_pendingPromotion;
    bool m_hasPendingPromotion = false;

    // 历史局面 FEN 计数（用于三次重复检测）
    QVector<QString> m_positionHistory;

    // ---- AI 状态 ----
    AIManager *m_aiManager = nullptr;
    bool m_whiteIsAI = false;
    bool m_blackIsAI = false;
    AIProvider m_whiteProvider;
    AIProvider m_blackProvider;

    bool m_aiThinking = false;
    QString m_aiThinkingName;
    QString m_aiThinkingModel;
    int m_aiRetryCount = 0;   // 当前 AI 走法重试次数
    QString m_aiPendingUci;   // 待验证的 AI 走法
};
