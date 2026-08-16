#pragma once

#include "Page.h"

#include "GameRecord.h"
#include "GameController.h"

class QLabel;
class QListWidget;
class QPushButton;
class QWidget;
class ChessBoard;

// 对局页面：棋盘 + 右侧信息区 + 升变/结算覆盖层。
// 负责对局进行、悔棋、新游戏、对局保存与 Continue Game。
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

private:
    void setupUi();
    QWidget *createSidePanel();
    QWidget *createInfoCard(const QString &title, const QString &value);
    QWidget *createMovePanel();
    QWidget *createActionButtons();
    QWidget *createPromotionOverlay();
    QWidget *createSettlementOverlay();

    // 信号槽
    void onMoveMade(int fromRow, int fromCol, int toRow, int toCol);
    void onPromotionRequired(int fromRow, int fromCol, int toRow, int toCol);
    void onGameStateChanged();
    void onGameOver(GameController::Result result, const QString &reason);
    void onUndoClicked();
    void onNewGameClicked();
    void onPromotionChosen(PieceType type);
    void onViewGameClicked();

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

    // 信息面板控件
    QLabel *m_turnValue = nullptr;
    QLabel *m_statusValue = nullptr;
    QLabel *m_lastMoveValue = nullptr;

    // 棋谱面板
    QListWidget *m_moveList = nullptr;

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
