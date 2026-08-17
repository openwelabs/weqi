#include "GamePage.h"

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QFont>
#include <QPushButton>
#include <QListWidget>
#include <QScrollBar>
#include <QMessageBox>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QTimer>

#include "MainWindow.h"
#include "ChessBoard.h"
#include "ChessPiece.h"
#include "GameController.h"
#include "MoveHistory.h"
#include "UiTheme.h"
#include "DataPaths.h"
#include "GameHistoryManager.h"
#include "ProfileManager.h"
#include "RatingSystem.h"

namespace {

// 棋盘格坐标 -> 代数记谱（如 e2）
QString squareName(int row, int col)
{
    const QChar file = QChar('a' + col);
    const int rank = 8 - row;
    return QString("%1%2").arg(file).arg(rank);
}

// 棋子类型 -> 显示符号
QString pieceSymbol(PieceType type)
{
    switch (type) {
    case PieceType::Queen:  return QStringLiteral("♕");
    case PieceType::Rook:   return QStringLiteral("♖");
    case PieceType::Bishop: return QStringLiteral("♗");
    case PieceType::Knight: return QStringLiteral("♘");
    default: return QString();
    }
}

} // namespace

GamePage::GamePage(MainWindow *window, QWidget *parent)
    : Page(window, parent)
{
    setupUi();
}

void GamePage::setupUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(28, 20, 28, 28);
    rootLayout->setSpacing(18);

    // ---- 顶部标题栏 ----
    auto *header = new QWidget(this);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(14);

    auto *backBtn = UiTheme::createGhostButton(QStringLiteral("← 首页"), header);
    connect(backBtn, &QPushButton::clicked, this, &GamePage::goHome);
    headerLayout->addWidget(backBtn);

    auto *titleLabel = UiTheme::createTitle(QStringLiteral("对局"), 20, header);
    headerLayout->addWidget(titleLabel);

    headerLayout->addStretch(1);
    rootLayout->addWidget(header);

    // ---- 主体：棋盘 + 右侧信息区 ----
    auto *bodyLayout = new QHBoxLayout();
    bodyLayout->setSpacing(24);

    // 棋盘容器：填满剩余空间，ChessBoard 内部保持正方形
    auto *boardContainer = new QWidget(this);
    auto *boardLayout = new QVBoxLayout(boardContainer);
    boardLayout->setContentsMargins(0, 0, 0, 0);
    boardLayout->setSpacing(10);
    // 白方 AI 聊天对话框（棋盘上方，默认隐藏）
    m_aiChatDialogWhite = createAIChatDialog(PieceColor::White);
    boardLayout->addWidget(m_aiChatDialogWhite);
    m_board = new ChessBoard(boardContainer);
    boardLayout->addWidget(m_board, 1);
    // 黑方 AI 聊天对话框（棋盘下方，默认隐藏）
    m_aiChatDialogBlack = createAIChatDialog(PieceColor::Black);
    boardLayout->addWidget(m_aiChatDialogBlack);
    // AI vs AI 控制条（默认隐藏，仅 AI vs AI 模式显示）
    m_aiVsAiControls = createAIVsAIControls();
    boardLayout->addWidget(m_aiVsAiControls);
    bodyLayout->addWidget(boardContainer, 1);

    // 右侧信息区
    bodyLayout->addWidget(createSidePanel(), 0);

    rootLayout->addLayout(bodyLayout, 1);

    // 绑定控制器
    m_board->setController(m_window->controller());

    // 连接信号
    connect(m_board, &ChessBoard::moveMade, this, &GamePage::onMoveMade);
    connect(m_board, &ChessBoard::promotionRequested, this, &GamePage::onPromotionRequired);
    connect(m_window->controller(), &GameController::promotionRequired,
            this, &GamePage::onPromotionRequired);
    connect(m_window->controller(), &GameController::gameStateChanged,
            this, &GamePage::onGameStateChanged);
    connect(m_window->controller(), &GameController::gameOver,
            this, &GamePage::onGameOver);

    // AI 相关信号
    connect(m_window->controller(), &GameController::aiThinkingChanged,
            this, &GamePage::onAIThinkingChanged);
    connect(m_window->controller(), &GameController::aiRequestFailed,
            this, &GamePage::onAIRequestFailed);
    connect(m_window->controller(), &GameController::aiMessageReady,
            this, &GamePage::onAIMessageReady);
    connect(m_window->controller(), &GameController::aiVsAiStateChanged,
            this, &GamePage::onAIVsAIStateChanged);

    // 初始化信息面板
    updateInfoPanel();
}

QWidget *GamePage::createSidePanel()
{
    auto *panel = new QFrame(this);
    panel->setFixedWidth(280);
    panel->setStyleSheet(QStringLiteral(
        "QFrame { background-color: %1; border-radius: 18px; }").arg(UiTheme::kPanelBg.name()));

    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(14);

    // 面板标题
    auto *sectionTitle = UiTheme::createSectionLabel(QStringLiteral("对局信息"), panel);
    layout->addWidget(sectionTitle);

    // 当前回合卡片
    auto *turnCard = createInfoCard(QStringLiteral("当前回合"), QStringLiteral("白方"));
    m_turnValue = turnCard->findChild<QLabel *>(QStringLiteral("valueLabel"));
    layout->addWidget(turnCard);

    // 对局状态卡片
    auto *statusCard = createInfoCard(QStringLiteral("对局状态"), QStringLiteral("进行中"));
    m_statusValue = statusCard->findChild<QLabel *>(QStringLiteral("valueLabel"));
    layout->addWidget(statusCard);

    // 最近一步卡片
    auto *lastMoveCard = createInfoCard(QStringLiteral("最近一步"), QStringLiteral("—"));
    m_lastMoveValue = lastMoveCard->findChild<QLabel *>(QStringLiteral("valueLabel"));
    layout->addWidget(lastMoveCard);

    // 棋谱面板
    layout->addWidget(createMovePanel());

    // 操作按钮（悔棋 / 新游戏）
    layout->addWidget(createActionButtons());

    // AI 思考面板（默认隐藏）
    layout->addWidget(createAIThinkingPanel());

    layout->addStretch(1);

    // 底部操作提示
    auto *hint = new QLabel(
        QStringLiteral("点击棋子选中，再点击目标格移动。"), panel);
    hint->setWordWrap(true);
    hint->setStyleSheet(QStringLiteral("color: %1;").arg(UiTheme::kMutedText.name()));
    layout->addWidget(hint);

    return panel;
}

// 创建信息卡片：标题 + 值
QWidget *GamePage::createInfoCard(const QString &title, const QString &value)
{
    auto *card = new QFrame(this);
    card->setStyleSheet(QStringLiteral(
        "QFrame { background-color: %1; border-radius: 14px; }").arg(UiTheme::kCardBg.name()));

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(4);

    auto *titleLabel = new QLabel(title, card);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(10);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet(QStringLiteral("color: %1;").arg(UiTheme::kMutedText.name()));
    layout->addWidget(titleLabel);

    auto *valueLabel = new QLabel(value, card);
    valueLabel->setObjectName(QStringLiteral("valueLabel"));
    QFont valueFont = valueLabel->font();
    valueFont.setPointSize(15);
    valueFont.setBold(true);
    valueLabel->setFont(valueFont);
    valueLabel->setStyleSheet(QStringLiteral("color: %1;").arg(UiTheme::kTitleColor.name()));
    layout->addWidget(valueLabel);

    return card;
}

// 棋谱面板：可滚动的走法列表
QWidget *GamePage::createMovePanel()
{
    auto *card = new QFrame(this);
    card->setStyleSheet(QStringLiteral(
        "QFrame { background-color: %1; border-radius: 14px; }").arg(UiTheme::kCardBg.name()));

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(6);

    auto *titleLabel = new QLabel(QStringLiteral("棋谱"), card);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet(QStringLiteral("color: %1;").arg(UiTheme::kTitleColor.name()));
    layout->addWidget(titleLabel);

    m_moveList = new QListWidget(card);
    m_moveList->setFixedHeight(180);
    m_moveList->setStyleSheet(QStringLiteral(
        "QListWidget { background-color: %1; border: none; border-radius: 10px;"
        " color: %2; font-size: 13px; padding: 6px; }")
        .arg(UiTheme::kPanelBg.name()).arg(UiTheme::kTitleColor.name()));
    m_moveList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    layout->addWidget(m_moveList);

    return card;
}

// 操作按钮：悔棋 + 新游戏
QWidget *GamePage::createActionButtons()
{
    auto *card = new QFrame(this);
    card->setStyleSheet(QStringLiteral(
        "QFrame { background-color: %1; border-radius: 14px; }").arg(UiTheme::kCardBg.name()));

    auto *layout = new QHBoxLayout(card);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    auto *undoBtn = new QPushButton(QStringLiteral("悔棋"), card);
    undoBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: %1; color: %2; border: none;"
        " border-radius: 10px; padding: 10px 0; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background-color: %3; }")
        .arg(UiTheme::kAccent.name()).arg(UiTheme::kTitleColor.name()).arg(UiTheme::kAccentSoft.name()));
    connect(undoBtn, &QPushButton::clicked, this, &GamePage::onUndoClicked);
    layout->addWidget(undoBtn, 1);

    auto *newGameBtn = new QPushButton(QStringLiteral("新游戏"), card);
    newGameBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: %1; color: %2; border: none;"
        " border-radius: 10px; padding: 10px 0; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background-color: %3; }")
        .arg(UiTheme::kCardBg.name()).arg(UiTheme::kTitleColor.name()).arg(UiTheme::kAccentSoft.name()));
    connect(newGameBtn, &QPushButton::clicked, this, &GamePage::onNewGameClicked);
    layout->addWidget(newGameBtn, 1);

    return card;
}

// 升变选择覆盖层：显示在棋盘上方，提供 ♕♖♗♘ 四个选择
QWidget *GamePage::createPromotionOverlay()
{
    auto *overlay = new QWidget(this);
    overlay->setStyleSheet(QStringLiteral(
        "QWidget { background-color: %1; border-radius: 18px; }").arg(UiTheme::kOverlayBg.name()));
    overlay->hide();

    auto *layout = new QVBoxLayout(overlay);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    auto *title = new QLabel(QStringLiteral("选择升变棋子"), overlay);
    QFont titleFont = title->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(QStringLiteral("color: %1;").arg(UiTheme::kTitleColor.name()));
    layout->addWidget(title);

    auto *row = new QHBoxLayout();
    row->setSpacing(12);

    const PieceType types[] = { PieceType::Queen, PieceType::Rook,
                                PieceType::Bishop, PieceType::Knight };
    for (PieceType type : types) {
        auto *btn = new QPushButton(pieceSymbol(type), overlay);
        btn->setFixedSize(64, 64);
        btn->setStyleSheet(QStringLiteral(
            "QPushButton { background-color: %1; color: %2; border: none;"
            " border-radius: 12px; font-size: 34px; }"
            "QPushButton:hover { background-color: %3; }")
            .arg(UiTheme::kCardBg.name()).arg(UiTheme::kTitleColor.name()).arg(UiTheme::kAccentSoft.name()));
        connect(btn, &QPushButton::clicked, this, [this, type]() {
            onPromotionChosen(type);
        });
        row->addWidget(btn);
    }

    layout->addLayout(row);
    return overlay;
}

// 结算覆盖层：显示将死/和棋结果与操作按钮
QWidget *GamePage::createSettlementOverlay()
{
    auto *overlay = new QWidget(this);
    overlay->setStyleSheet(QStringLiteral(
        "QWidget { background-color: %1; border-radius: 18px; }").arg(UiTheme::kOverlayBg.name()));
    overlay->hide();

    auto *layout = new QVBoxLayout(overlay);
    layout->setContentsMargins(32, 32, 32, 32);
    layout->setSpacing(14);

    m_settlementTitle = new QLabel(overlay);
    QFont titleFont = m_settlementTitle->font();
    titleFont.setPointSize(28);
    titleFont.setBold(true);
    m_settlementTitle->setFont(titleFont);
    m_settlementTitle->setAlignment(Qt::AlignCenter);
    m_settlementTitle->setStyleSheet(QStringLiteral("color: %1;").arg(UiTheme::kTitleColor.name()));
    layout->addWidget(m_settlementTitle);

    m_settlementSubtitle = new QLabel(overlay);
    QFont subFont = m_settlementSubtitle->font();
    subFont.setPointSize(14);
    m_settlementSubtitle->setFont(subFont);
    m_settlementSubtitle->setAlignment(Qt::AlignCenter);
    m_settlementSubtitle->setStyleSheet(QStringLiteral("color: %1;").arg(UiTheme::kMutedText.name()));
    layout->addWidget(m_settlementSubtitle);

    auto *row = new QHBoxLayout();
    row->setSpacing(12);

    auto *newGameBtn = new QPushButton(QStringLiteral("新游戏"), overlay);
    newGameBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: %1; color: %2; border: none;"
        " border-radius: 10px; padding: 10px 24px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background-color: %3; }")
        .arg(UiTheme::kAccent.name()).arg(UiTheme::kTitleColor.name()).arg(UiTheme::kAccentSoft.name()));
    connect(newGameBtn, &QPushButton::clicked, this, &GamePage::onNewGameClicked);
    row->addWidget(newGameBtn);

    auto *viewBtn = new QPushButton(QStringLiteral("查看棋谱"), overlay);
    viewBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: %1; color: %2; border: none;"
        " border-radius: 10px; padding: 10px 24px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background-color: %3; }")
        .arg(UiTheme::kCardBg.name()).arg(UiTheme::kTitleColor.name()).arg(UiTheme::kAccentSoft.name()));
    connect(viewBtn, &QPushButton::clicked, this, &GamePage::onViewGameClicked);
    row->addWidget(viewBtn);

    layout->addLayout(row);
    return overlay;
}

// AI 思考面板：显示 AI 名称/模型、思考动画与取消按钮
QWidget *GamePage::createAIThinkingPanel()
{
    auto *panel = new QFrame(this);
    panel->setStyleSheet(QStringLiteral(
        "QFrame { background-color: %1; border-radius: 14px; }").arg(UiTheme::kCardBg.name()));
    panel->hide();

    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(6);

    // 思考状态标题（带动态省略号）
    m_aiThinkingLabel = new QLabel(QStringLiteral("AI 思考中…"), panel);
    QFont titleFont = m_aiThinkingLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    m_aiThinkingLabel->setFont(titleFont);
    m_aiThinkingLabel->setStyleSheet(QStringLiteral("color: %1;").arg(UiTheme::kAccent.name()));
    layout->addWidget(m_aiThinkingLabel);

    // AI 名称与模型
    m_aiThinkingModelLabel = new QLabel(panel);
    QFont modelFont = m_aiThinkingModelLabel->font();
    modelFont.setPointSize(11);
    m_aiThinkingModelLabel->setFont(modelFont);
    m_aiThinkingModelLabel->setWordWrap(true);
    m_aiThinkingModelLabel->setStyleSheet(QStringLiteral("color: %1;").arg(UiTheme::kMutedText.name()));
    layout->addWidget(m_aiThinkingModelLabel);

    // 取消按钮
    m_cancelAIButton = new QPushButton(QStringLiteral("取消"), panel);
    m_cancelAIButton->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: %1; color: %2; border: none;"
        " border-radius: 10px; padding: 8px 0; font-size: 13px; font-weight: bold; }"
        "QPushButton:hover { background-color: %3; }")
        .arg(UiTheme::kDanger.name()).arg(UiTheme::kTitleColor.name()).arg(UiTheme::kAccentSoft.name()));
    connect(m_cancelAIButton, &QPushButton::clicked, this, &GamePage::onCancelAIClicked);
    layout->addWidget(m_cancelAIButton);

    // 思考动画定时器（循环省略号）
    m_aiThinkingAnimTimer = new QTimer(this);
    m_aiThinkingAnimTimer->setInterval(400);
    connect(m_aiThinkingAnimTimer, &QTimer::timeout, this, [this]() {
        m_aiThinkingDotCount = (m_aiThinkingDotCount + 1) % 4;
        QString dots;
        for (int i = 0; i < m_aiThinkingDotCount; ++i)
            dots += QChar('.');
        m_aiThinkingLabel->setText(QStringLiteral("AI 思考中%1").arg(dots));
    });

    return panel;
}

// AI 聊天对话框：棋盘上方/下方的一张轻量信息卡，显示模型名 + 一句聊天内容。
// 白方与黑方各一个，通过 color 区分。默认隐藏，仅在 AI 走法合法并执行后淡入显示，随后淡出。
QWidget *GamePage::createAIChatDialog(PieceColor color)
{
    auto *dialog = new QFrame(this);
    dialog->setStyleSheet(QStringLiteral(
        "QFrame { background-color: %1; border-radius: 14px; }").arg(UiTheme::kCardBg.name()));
    dialog->hide();

    auto *layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(16, 10, 16, 10);
    layout->setSpacing(4);

    // 标题：当前 AI 模型名（动态更新）
    auto *titleLabel = new QLabel(dialog);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet(QStringLiteral("color: %1;").arg(UiTheme::kAccent.name()));
    layout->addWidget(titleLabel);

    // 消息内容
    auto *msgLabel = new QLabel(dialog);
    QFont msgFont = msgLabel->font();
    msgFont.setPointSize(13);
    msgLabel->setFont(msgFont);
    msgLabel->setWordWrap(true);
    msgLabel->setStyleSheet(QStringLiteral("color: %1;").arg(UiTheme::kTitleColor.name()));
    layout->addWidget(msgLabel);

    // 绑定到对应方
    if (color == PieceColor::White) {
        m_aiChatTitleWhite = titleLabel;
        m_aiChatMessageWhite = msgLabel;
    } else {
        m_aiChatTitleBlack = titleLabel;
        m_aiChatMessageBlack = msgLabel;
    }

    return dialog;
}

// AI vs AI 控制条：Start / Pause / Resume / Stop 四个按钮。
// 仅在 AI vs AI 模式下显示，用于控制自动对战流程。
QWidget *GamePage::createAIVsAIControls()
{
    auto *card = new QFrame(this);
    card->setStyleSheet(QStringLiteral(
        "QFrame { background-color: %1; border-radius: 14px; }").arg(UiTheme::kCardBg.name()));
    card->hide();

    auto *layout = new QHBoxLayout(card);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(8);

    auto makeBtn = [this, card](const QString &text) {
        auto *btn = new QPushButton(text, card);
        btn->setStyleSheet(QStringLiteral(
            "QPushButton { background-color: %1; color: %2; border: none;"
            " border-radius: 10px; padding: 8px 0; font-size: 13px; font-weight: bold; }"
            "QPushButton:hover { background-color: %3; }"
            "QPushButton:disabled { color: %4; background-color: %5; }")
            .arg(UiTheme::kAccent.name()).arg(UiTheme::kTitleColor.name())
            .arg(UiTheme::kAccentSoft.name()).arg(UiTheme::kMutedText.name())
            .arg(UiTheme::kPanelBg.name()));
        return btn;
    };

    m_aiVsAiStartBtn = makeBtn(QStringLiteral("开始"));
    connect(m_aiVsAiStartBtn, &QPushButton::clicked, this, &GamePage::onAIVsAIStartClicked);
    layout->addWidget(m_aiVsAiStartBtn, 1);

    m_aiVsAiPauseBtn = makeBtn(QStringLiteral("暂停"));
    connect(m_aiVsAiPauseBtn, &QPushButton::clicked, this, &GamePage::onAIVsAIPauseClicked);
    layout->addWidget(m_aiVsAiPauseBtn, 1);

    m_aiVsAiResumeBtn = makeBtn(QStringLiteral("继续"));
    connect(m_aiVsAiResumeBtn, &QPushButton::clicked, this, &GamePage::onAIVsAIResumeClicked);
    layout->addWidget(m_aiVsAiResumeBtn, 1);

    m_aiVsAiStopBtn = makeBtn(QStringLiteral("停止"));
    connect(m_aiVsAiStopBtn, &QPushButton::clicked, this, &GamePage::onAIVsAIStopClicked);
    layout->addWidget(m_aiVsAiStopBtn, 1);

    return card;
}

// ---- 对局流程 ----

void GamePage::startNewGame(GameMode mode, const QString &opponent,
                            const QString &whiteName, const QString &blackName)
{
    m_mode = mode;
    m_opponent = opponent;
    m_whiteName = whiteName;
    m_blackName = blackName;

    m_window->controller()->newGame();
    hideSettlement();
    if (m_aiChatDialogWhite)
        m_aiChatDialogWhite->hide();
    if (m_aiChatDialogBlack)
        m_aiChatDialogBlack->hide();

    // AI vs AI 模式：显示控制条，初始为 Stopped（等待 Start）
    if (m_aiVsAiControls) {
        m_aiVsAiControls->setVisible(m_mode == GameMode::AIVsAI);
        if (m_mode == GameMode::AIVsAI)
            onAIVsAIStateChanged(false, false);
    }

    updateInfoPanel();
    updateMovePanel();
}

void GamePage::continueSavedGame()
{
    // 读取 saves/current_game.json
    const QString path = DataPaths::currentGameFile();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
    file.close();

    const QString fen = obj.value(QStringLiteral("fen")).toString();
    if (fen.isEmpty())
        return;

    // 恢复对局元信息
    m_mode = static_cast<GameMode>(obj.value(QStringLiteral("mode")).toInt(0));
    m_opponent = obj.value(QStringLiteral("opponent")).toString();
    m_whiteName = obj.value(QStringLiteral("whiteName")).toString();
    m_blackName = obj.value(QStringLiteral("blackName")).toString();

    // 恢复棋盘状态
    GameController *controller = m_window->controller();
    if (!controller->loadFen(fen))
        return;

    hideSettlement();
    if (m_aiChatDialogWhite)
        m_aiChatDialogWhite->hide();
    if (m_aiChatDialogBlack)
        m_aiChatDialogBlack->hide();
    updateInfoPanel();
    updateMovePanel();
}

void GamePage::goHome()
{
    // 若对局进行中，先保存当前进度（Continue Game）
    if (m_window->controller()->result() == GameController::Result::Ongoing)
        saveCurrentGame();
    m_window->showHome();
}

void GamePage::onShown()
{
    updateInfoPanel();
    updateMovePanel();
}

// ---- 信号槽 ----

void GamePage::onMoveMade(int fromRow, int fromCol, int toRow, int toCol)
{
    // 更新最近一步
    const QString move = QStringLiteral("%1 → %2")
                             .arg(squareName(fromRow, fromCol))
                             .arg(squareName(toRow, toCol));
    if (m_lastMoveValue)
        m_lastMoveValue->setText(move);

    // 更新棋谱面板
    updateMovePanel();

    // 回合状态由控制器维护
    updateInfoPanel();
}

void GamePage::onPromotionRequired(int fromRow, int fromCol, int toRow, int toCol)
{
    m_promotionFromRow = fromRow;
    m_promotionFromCol = fromCol;
    m_promotionToRow = toRow;
    m_promotionToCol = toCol;

    if (!m_promotionOverlay) {
        m_promotionOverlay = createPromotionOverlay();
        m_promotionOverlay->setParent(this);
    }
    // 覆盖在棋盘区域中央
    const QRect boardGeo = m_board->geometry();
    const QPoint boardTopLeft = m_board->mapTo(this, QPoint(0, 0));
    m_promotionOverlay->setGeometry(QRect(boardTopLeft, boardGeo.size()));
    m_promotionOverlay->raise();
    m_promotionOverlay->show();
}

void GamePage::onPromotionChosen(PieceType type)
{
    if (m_promotionOverlay)
        m_promotionOverlay->hide();
    m_window->controller()->completePromotion(type);
}

void GamePage::onGameStateChanged()
{
    updateInfoPanel();
    updateMovePanel();
}

void GamePage::onGameOver(GameController::Result result, const QString &reason)
{
    // 保存对局记录并应用 Rating 变化
    saveGame(result, reason);
    showSettlement(result, reason);
}

void GamePage::onUndoClicked()
{
    m_window->controller()->undo();
}

void GamePage::onNewGameClicked()
{
    // 若对局已结束，直接开始新游戏；否则弹出确认对话框
    if (m_window->controller()->result() != GameController::Result::Ongoing) {
        m_window->controller()->newGame();
        hideSettlement();
        return;
    }

    const auto ret = QMessageBox::question(
        this, QStringLiteral("新游戏"),
        QStringLiteral("确定要开始新游戏吗？当前对局进度将丢失。"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        m_window->controller()->newGame();
        hideSettlement();
    }
}

void GamePage::onViewGameClicked()
{
    // 查看棋谱：滚动到棋谱面板并高亮
    if (m_moveList) {
        m_moveList->setFocus();
        m_moveList->scrollToBottom();
    }
    hideSettlement();
}

// ---- AI 相关 ----

void GamePage::onAIThinkingChanged(bool thinking, const QString &name, const QString &model)
{
    if (!m_aiThinkingPanel)
        return;

    if (thinking) {
        // 显示 AI 名称与模型
        QString modelText = model;
        if (modelText.isEmpty())
            modelText = QStringLiteral("未知模型");
        m_aiThinkingModelLabel->setText(
            QStringLiteral("%1  ·  %2").arg(name, modelText));

        // 同步更新聊天对话框标题：优先使用用户填写的 Provider 名称（"名称"字段），
        // 其次模型 API 名，最后回退到 "AI"。不自动从 Provider 类型推断。
        const QString title = !name.isEmpty() ? name
                            : (!model.isEmpty() ? model
                            : QStringLiteral("AI"));

        // 根据当前回合方更新对应聊天框标题
        GameController *controller = m_window->controller();
        if (controller && controller->currentTurn() == PieceColor::White) {
            m_aiChatModelNameWhite = title;
            if (m_aiChatTitleWhite)
                m_aiChatTitleWhite->setText(title);
        } else {
            m_aiChatModelNameBlack = title;
            if (m_aiChatTitleBlack)
                m_aiChatTitleBlack->setText(title);
        }

        m_aiThinkingDotCount = 0;
        m_aiThinkingLabel->setText(QStringLiteral("AI 思考中"));
        m_aiThinkingPanel->show();
        m_aiThinkingAnimTimer->start();
    } else {
        m_aiThinkingAnimTimer->stop();
        m_aiThinkingPanel->hide();
    }
}

void GamePage::onAIMessageReady(const QString &message, PieceColor color)
{
    // 选择对应方的聊天对话框
    QWidget *dialog = (color == PieceColor::White) ? m_aiChatDialogWhite : m_aiChatDialogBlack;
    QLabel *msgLabel = (color == PieceColor::White) ? m_aiChatMessageWhite : m_aiChatMessageBlack;
    if (!dialog || !msgLabel || message.isEmpty())
        return;

    // 设置消息内容
    msgLabel->setText(message);

    // 淡入显示
    dialog->show();
    dialog->raise();
    auto *effect = new QGraphicsOpacityEffect(dialog);
    dialog->setGraphicsEffect(effect);
    auto *fadeIn = new QPropertyAnimation(effect, "opacity", this);
    fadeIn->setDuration(250);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::OutCubic);
    fadeIn->start(QAbstractAnimation::DeleteWhenStopped);

    // 停留片刻后淡出
    QTimer::singleShot(4000, this, [this, dialog]() {
        if (!dialog || !dialog->isVisible())
            return;
        auto *effect = new QGraphicsOpacityEffect(dialog);
        dialog->setGraphicsEffect(effect);
        auto *fadeOut = new QPropertyAnimation(effect, "opacity", this);
        fadeOut->setDuration(400);
        fadeOut->setStartValue(1.0);
        fadeOut->setEndValue(0.0);
        fadeOut->setEasingCurve(QEasingCurve::InCubic);
        connect(fadeOut, &QPropertyAnimation::finished, this, [dialog]() {
            if (dialog)
                dialog->hide();
        });
        fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

void GamePage::onAIRequestFailed(const QString &error)
{
    // 停止思考动画
    if (m_aiThinkingAnimTimer)
        m_aiThinkingAnimTimer->stop();
    if (m_aiThinkingPanel)
        m_aiThinkingPanel->hide();
    // AI 请求失败时不显示聊天内容
    if (m_aiChatDialogWhite)
        m_aiChatDialogWhite->hide();
    if (m_aiChatDialogBlack)
        m_aiChatDialogBlack->hide();

    m_lastAIError = error;

    // 显示失败覆盖层
    if (!m_aiErrorOverlay) {
        m_aiErrorOverlay = new QWidget(this);
        m_aiErrorOverlay->setStyleSheet(QStringLiteral(
            "QWidget { background-color: %1; border-radius: 18px; }").arg(UiTheme::kOverlayBg.name()));

        auto *layout = new QVBoxLayout(m_aiErrorOverlay);
        layout->setContentsMargins(32, 32, 32, 32);
        layout->setSpacing(14);

        auto *title = new QLabel(QStringLiteral("AI 请求失败"), m_aiErrorOverlay);
        QFont titleFont = title->font();
        titleFont.setPointSize(20);
        titleFont.setBold(true);
        title->setFont(titleFont);
        title->setAlignment(Qt::AlignCenter);
        title->setStyleSheet(QStringLiteral("color: %1;").arg(UiTheme::kDanger.name()));
        layout->addWidget(title);

        m_aiErrorLabel = new QLabel(m_aiErrorOverlay);
        m_aiErrorLabel->setWordWrap(true);
        m_aiErrorLabel->setAlignment(Qt::AlignCenter);
        m_aiErrorLabel->setStyleSheet(QStringLiteral("color: %1;").arg(UiTheme::kMutedText.name()));
        layout->addWidget(m_aiErrorLabel);

        auto *row = new QHBoxLayout();
        row->setSpacing(12);

        auto *retryBtn = new QPushButton(QStringLiteral("重试"), m_aiErrorOverlay);
        retryBtn->setStyleSheet(QStringLiteral(
            "QPushButton { background-color: %1; color: %2; border: none;"
            " border-radius: 10px; padding: 10px 24px; font-size: 14px; font-weight: bold; }"
            "QPushButton:hover { background-color: %3; }")
            .arg(UiTheme::kAccent.name()).arg(UiTheme::kTitleColor.name()).arg(UiTheme::kAccentSoft.name()));
        connect(retryBtn, &QPushButton::clicked, this, &GamePage::onRetryAIClicked);
        row->addWidget(retryBtn);

        auto *cancelBtn = new QPushButton(QStringLiteral("取消"), m_aiErrorOverlay);
        cancelBtn->setStyleSheet(QStringLiteral(
            "QPushButton { background-color: %1; color: %2; border: none;"
            " border-radius: 10px; padding: 10px 24px; font-size: 14px; font-weight: bold; }"
            "QPushButton:hover { background-color: %3; }")
            .arg(UiTheme::kCardBg.name()).arg(UiTheme::kTitleColor.name()).arg(UiTheme::kAccentSoft.name()));
        connect(cancelBtn, &QPushButton::clicked, this, &GamePage::onAICancelClicked);
        row->addWidget(cancelBtn);

        layout->addLayout(row);
    }

    m_aiErrorLabel->setText(error.isEmpty() ? QStringLiteral("未知错误") : error);

    // 覆盖在棋盘区域中央
    const QRect boardGeo = m_board->geometry();
    const QPoint boardTopLeft = m_board->mapTo(this, QPoint(0, 0));
    m_aiErrorOverlay->setGeometry(QRect(boardTopLeft, boardGeo.size()));
    m_aiErrorOverlay->raise();
    m_aiErrorOverlay->show();
}

void GamePage::onCancelAIClicked()
{
    // 取消当前 AI 请求
    m_window->controller()->cancelAI();
}

void GamePage::onRetryAIClicked()
{
    if (m_aiErrorOverlay)
        m_aiErrorOverlay->hide();

    // 重新触发 AI 走子
    m_window->controller()->retryAI();
}

void GamePage::onAICancelClicked()
{
    if (m_aiErrorOverlay)
        m_aiErrorOverlay->hide();
}

// ---- AI vs AI 控制 ----

void GamePage::onAIVsAIStartClicked()
{
    m_window->controller()->startAIVsAI();
}

void GamePage::onAIVsAIPauseClicked()
{
    m_window->controller()->pauseAIVsAI();
}

void GamePage::onAIVsAIResumeClicked()
{
    m_window->controller()->resumeAIVsAI();
}

void GamePage::onAIVsAIStopClicked()
{
    m_window->controller()->stopAIVsAI();
}

void GamePage::onAIVsAIStateChanged(bool running, bool paused)
{
    if (!m_aiVsAiControls)
        return;

    // 根据状态启用/禁用按钮
    if (m_aiVsAiStartBtn)
        m_aiVsAiStartBtn->setEnabled(!running);
    if (m_aiVsAiPauseBtn)
        m_aiVsAiPauseBtn->setEnabled(running && !paused);
    if (m_aiVsAiResumeBtn)
        m_aiVsAiResumeBtn->setEnabled(running && paused);
    if (m_aiVsAiStopBtn)
        m_aiVsAiStopBtn->setEnabled(running);
}

// ---- 更新信息面板 ----

void GamePage::updateInfoPanel()
{
    GameController *controller = m_window->controller();
    if (!controller)
        return;

    // 当前回合：显示当前回合方，若为 AI 则显示其名称（用户填写的"名称"字段）
    if (m_turnValue) {
        const bool whiteTurn = (controller->currentTurn() == PieceColor::White);
        QString turnText = whiteTurn ? QStringLiteral("白方") : QStringLiteral("黑方");

        // 若当前回合方为 AI，附加其名称
        if (controller->isAIVsAI()) {
            const QString aiName = whiteTurn
                                       ? controller->whiteAIProviderName()
                                       : controller->blackAIProviderName();
            if (!aiName.isEmpty())
                turnText = QStringLiteral("%1 · %2").arg(turnText, aiName);
        } else if (whiteTurn && controller->isWhiteAI()) {
            const QString aiName = controller->whiteAIProviderName();
            if (!aiName.isEmpty())
                turnText = QStringLiteral("%1 · %2").arg(turnText, aiName);
        } else if (!whiteTurn && controller->isBlackAI()) {
            const QString aiName = controller->blackAIProviderName();
            if (!aiName.isEmpty())
                turnText = QStringLiteral("%1 · %2").arg(turnText, aiName);
        }

        m_turnValue->setText(turnText);
    }

    // 对局状态
    if (m_statusValue) {
        m_statusValue->setText(controller->statusText());
    }
}

void GamePage::updateMovePanel()
{
    if (!m_moveList)
        return;

    GameController *controller = m_window->controller();
    if (!controller)
        return;

    m_moveList->clear();
    const MoveHistory &history = controller->history();
    for (int i = 0; i < history.count(); ++i) {
        const MoveRecord &rec = history.at(i);
        const int moveNumber = (i / 2) + 1;
        const QString prefix = (rec.move.color == PieceColor::White)
                                   ? QStringLiteral("%1. ").arg(moveNumber)
                                   : QStringLiteral("   ");
        m_moveList->addItem(prefix + rec.san);
    }
    m_moveList->scrollToBottom();
}

// ---- 结算覆盖层 ----

void GamePage::showSettlement(GameController::Result result, const QString &reason)
{
    if (!m_settlementOverlay) {
        m_settlementOverlay = createSettlementOverlay();
        m_settlementOverlay->setParent(this);
    }

    QString title;
    QString subtitle;
    switch (result) {
    case GameController::Result::WhiteWin:
        title = QStringLiteral("CHECKMATE");
        subtitle = QStringLiteral("白方获胜  ♔ 1-0");
        break;
    case GameController::Result::BlackWin:
        title = QStringLiteral("CHECKMATE");
        subtitle = QStringLiteral("黑方获胜  ♚ 0-1");
        break;
    case GameController::Result::Draw:
        title = QStringLiteral("DRAW");
        subtitle = QStringLiteral("和棋  %1").arg(reason);
        break;
    default:
        return;
    }

    m_settlementTitle->setText(title);
    m_settlementSubtitle->setText(subtitle);

    // 覆盖在棋盘区域中央
    const QRect boardGeo = m_board->geometry();
    const QPoint boardTopLeft = m_board->mapTo(this, QPoint(0, 0));
    m_settlementOverlay->setGeometry(QRect(boardTopLeft, boardGeo.size()));
    m_settlementOverlay->raise();
    m_settlementOverlay->show();

    // 淡入动画
    auto *effect = new QGraphicsOpacityEffect(m_settlementOverlay);
    m_settlementOverlay->setGraphicsEffect(effect);
    auto *anim = new QPropertyAnimation(effect, "opacity", this);
    anim->setDuration(300);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void GamePage::hideSettlement()
{
    if (m_settlementOverlay)
        m_settlementOverlay->hide();
}

// ---- 对局保存 ----

void GamePage::saveGame(GameController::Result result, const QString &reason)
{
    GameController *controller = m_window->controller();

    // 构建对局记录
    GameRecord record;
    record.id = QString::number(QDateTime::currentMSecsSinceEpoch());
    record.date = QDateTime::currentDateTime();
    record.mode = m_mode;
    record.opponent = m_opponent;
    record.whiteName = m_whiteName;
    record.blackName = m_blackName;
    record.moveCount = controller->history().count();
    record.reason = reason;

    // AI 对局信息
    record.whiteAIProvider = controller->whiteAIProviderName();
    record.whiteAIModel = controller->whiteAIModel();
    record.blackAIProvider = controller->blackAIProviderName();
    record.blackAIModel = controller->blackAIModel();

    // 结果文本与玩家视角结果
    const QString playerName = m_window->profile()->playerName();
    const bool playerIsWhite = (m_whiteName == playerName);
    switch (result) {
    case GameController::Result::WhiteWin:
        record.resultText = QStringLiteral("1-0");
        // AI vs AI 不计入玩家战绩，胜负仅作汇报（outcome 保持 Ongoing）
        record.outcome = (m_mode == GameMode::AIVsAI)
                             ? GameOutcome::Ongoing
                             : (playerIsWhite ? GameOutcome::Win : GameOutcome::Loss);
        break;
    case GameController::Result::BlackWin:
        record.resultText = QStringLiteral("0-1");
        record.outcome = (m_mode == GameMode::AIVsAI)
                             ? GameOutcome::Ongoing
                             : (playerIsWhite ? GameOutcome::Loss : GameOutcome::Win);
        break;
    case GameController::Result::Draw:
        record.resultText = QStringLiteral("½-½");
        record.outcome = GameOutcome::Draw;
        break;
    default:
        record.outcome = GameOutcome::Ongoing;
        break;
    }

    // 生成 PGN（简化：仅记录走法）
    QString pgn;
    const MoveHistory &history = controller->history();
    for (int i = 0; i < history.count(); ++i) {
        const MoveRecord &rec = history.at(i);
        if (rec.move.color == PieceColor::White)
            pgn += QStringLiteral("%1. ").arg((i / 2) + 1);
        pgn += rec.san + QStringLiteral(" ");
    }
    record.pgn = pgn.trimmed();

    // 应用 Rating 变化（仅对含玩家的对局）
    if (m_mode == GameMode::HumanVsHuman || m_mode == GameMode::HumanVsAI) {
        double resultScore = 0.5;
        if (record.outcome == GameOutcome::Win)
            resultScore = 1.0;
        else if (record.outcome == GameOutcome::Loss)
            resultScore = 0.0;

        const int playerRating = m_window->profile()->rating();
        const int opponentRating = 1000; // 本地对局对手 Rating 暂用默认值
        const int delta = RatingSystem::ratingDelta(playerRating, opponentRating, resultScore);
        record.ratingChange = delta;
        m_window->profile()->applyRatingChange(delta);
    }

    // 写入历史
    m_window->history()->addRecord(record);

    // 清除未完成对局存档
    clearSavedGame();
}

void GamePage::saveCurrentGame()
{
    GameController *controller = m_window->controller();

    QJsonObject obj;
    obj[QStringLiteral("mode")] = static_cast<int>(m_mode);
    obj[QStringLiteral("opponent")] = m_opponent;
    obj[QStringLiteral("whiteName")] = m_whiteName;
    obj[QStringLiteral("blackName")] = m_blackName;
    obj[QStringLiteral("fen")] = controller->state().toFen();

    const QString path = DataPaths::currentGameFile();
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        file.close();
    }
}

void GamePage::clearSavedGame()
{
    const QString path = DataPaths::currentGameFile();
    if (QFile::exists(path))
        QFile::remove(path);
}
