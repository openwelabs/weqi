#pragma once

#include "Page.h"

#include "GameRecord.h"
#include "GameController.h"

class QLabel;
class QListWidget;
class QPushButton;
class QWidget;
class QTimer;
class ChessBoard;

// 对局页面：棋盘 + 右侧信息区 + 升变/结算覆盖层。
// 负责对局进行、悔棋、新游戏、对局保存与 Continue Game。
// 支持 AI 对局：显示 AI 思考状态、AI 名称/模型、取消按钮。
class GamePage : public Page
{
    Q_OBJECT

public:
    explicit GamePage(MainWindow *window, QWidget *parent = nullptr);

    // 开始一局新游戏
    void startNewGame(GameMode mode, const QString &opponent,
                      const QString &whiteName, const QString &blackName);

    // 继续未完成对局
    void continueSavedGame();

    // 返回首页
    void goHome();

    void onShown() override;
    void retranslateUi() override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setupUi();
    QWidget *createSidePanel();
    QWidget *createInfoCard(const QString &title, const QString &value);
    QWidget *createMovePanel();
    QWidget *createActionButtons();
    QWidget *createPromotionOverlay();
    QWidget *createSettlementOverlay();
    QWidget *createAIThinkingPanel();
    QWidget *createAIChatDialog(PieceColor color);
    QWidget *createAIVsAIControls();

    // 信号槽
    void onMoveMade(int fromRow, int fromCol, int toRow, int toCol);
    void onPromotionRequired(int fromRow, int fromCol, int toRow, int toCol);
    void onGameStateChanged();
    void onGameOver(GameController::Result result, const QString &reason);
    void onUndoClicked();
    void onNewGameClicked();
    void onPromotionChosen(PieceType type);
    void onViewGameClicked();

    // AI 相关
    void onAIThinkingChanged(bool thinking, const QString &name, const QString &model);
    void onAIRequestFailed(const QString &error);
    void onCancelAIClicked();
    void onRetryAIClicked();
    void onAICancelClicked();
    void onAIMessageReady(const QString &message, PieceColor color);
    void onAIVsAIStateChanged(bool running, bool paused);

    // AI vs AI 控制按钮
    void onAIVsAIStartClicked();
    void onAIVsAIPauseClicked();
    void onAIVsAIResumeClicked();
    void onAIVsAIStopClicked();

    // 更新信息面板
    void updateInfoPanel();
    void updateMovePanel();

    // 结算覆盖层显示/隐藏
    void showSettlement(GameController::Result result, const QString &reason);
    void hideSettlement();

    // 对局保存
    void saveGame(GameController::Result result, const QString &reason);
    void saveCurrentGame();   // 保存未完成对局（Continue Game）
    void clearSavedGame();

    // 对局元信息
    GameMode m_mode = GameMode::HumanVsHuman;
    QString m_opponent;
    QString m_whiteName;
    QString m_blackName;

    ChessBoard *m_board = nullptr;

    // 顶部标题栏
    QPushButton *m_backBtn = nullptr;
    QLabel *m_headerTitle = nullptr;

    // 信息面板控件
    QLabel *m_sectionTitle = nullptr;
    QLabel *m_turnTitle = nullptr;
    QLabel *m_statusTitle = nullptr;
    QLabel *m_lastMoveTitle = nullptr;
    QLabel *m_turnValue = nullptr;
    QLabel *m_statusValue = nullptr;
    QLabel *m_lastMoveValue = nullptr;
    QLabel *m_hintLabel = nullptr;

    // 棋谱面板
    QListWidget *m_moveList = nullptr;
    QLabel *m_movePanelTitle = nullptr;

    // 操作按钮
    QPushButton *m_undoBtn = nullptr;
    QPushButton *m_newGameBtn = nullptr;

    // 升变覆盖层标题
    QLabel *m_promotionTitle = nullptr;

    // 结算覆盖层按钮
    QPushButton *m_settlementNewGameBtn = nullptr;
    QPushButton *m_settlementViewBtn = nullptr;

    // AI 思考面板
    QWidget *m_aiThinkingPanel = nullptr;
    QLabel *m_aiThinkingLabel = nullptr;
    QLabel *m_aiThinkingModelLabel = nullptr;
    QPushButton *m_cancelAIButton = nullptr;
    QTimer *m_aiThinkingAnimTimer = nullptr;
    int m_aiThinkingDotCount = 0;

    // AI 聊天对话框（棋盘上方，显示模型名 + 一句聊天内容）
    // 白方与黑方各一个，独立显示，互不干扰。
    QWidget *m_aiChatDialogWhite = nullptr;
    QLabel *m_aiChatTitleWhite = nullptr;
    QLabel *m_aiChatMessageWhite = nullptr;
    QWidget *m_aiChatDialogBlack = nullptr;
    QLabel *m_aiChatTitleBlack = nullptr;
    QLabel *m_aiChatMessageBlack = nullptr;
    QString m_aiChatModelNameWhite;   // 白方 AI 模型名（用于标题，动态更新）
    QString m_aiChatModelNameBlack;   // 黑方 AI 模型名（用于标题，动态更新）

    // AI vs AI 控制按钮
    QWidget *m_aiVsAiControls = nullptr;
    QPushButton *m_aiVsAiStartBtn = nullptr;
    QPushButton *m_aiVsAiPauseBtn = nullptr;
    QPushButton *m_aiVsAiResumeBtn = nullptr;
    QPushButton *m_aiVsAiStopBtn = nullptr;

    // AI 失败对话框（用于重试/取消）
    QWidget *m_aiErrorOverlay = nullptr;
    QLabel *m_aiErrorTitle = nullptr;
    QLabel *m_aiErrorLabel = nullptr;
    QPushButton *m_aiErrorRetryBtn = nullptr;
    QPushButton *m_aiErrorCancelBtn = nullptr;
    QString m_lastAIError;

    // 升变覆盖层
    QWidget *m_promotionOverlay = nullptr;
    int m_promotionFromRow = -1;
    int m_promotionFromCol = -1;
    int m_promotionToRow = -1;
    int m_promotionToCol = -1;

    // 结算覆盖层
    QWidget *m_settlementOverlay = nullptr;
    QLabel *m_settlementTitle = nullptr;
    QLabel *m_settlementSubtitle = nullptr;
};
