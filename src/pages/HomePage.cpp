#include "HomePage.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QScrollArea>
#include <QDateTime>
#include <QFile>
#include <QEvent>
#include <QMouseEvent>
#include <functional>

#include "MainWindow.h"
#include "UiTheme.h"
#include "ProfileManager.h"
#include "StatsManager.h"
#include "GameHistoryManager.h"
#include "DataPaths.h"
#include "GameRecord.h"

#include <QCoreApplication>

namespace {

// 对局类型 -> 显示文本
QString modeText(GameMode mode)
{
    switch (mode) {
    case GameMode::HumanVsHuman: return QCoreApplication::translate("HomePage", "真人对战");
    case GameMode::HumanVsAI:    return QCoreApplication::translate("HomePage", "人机对战");
    case GameMode::AIVsAI:       return QCoreApplication::translate("HomePage", "AI 对战");
    case GameMode::Replay:       return QCoreApplication::translate("HomePage", "复盘");
    }
    return QString();
}

// 结果 -> 显示文本
QString outcomeText(GameOutcome outcome)
{
    switch (outcome) {
    case GameOutcome::Win:    return QCoreApplication::translate("HomePage", "胜");
    case GameOutcome::Loss:   return QCoreApplication::translate("HomePage", "负");
    case GameOutcome::Draw:   return QCoreApplication::translate("HomePage", "和");
    case GameOutcome::Ongoing: return QCoreApplication::translate("HomePage", "进行中");
    }
    return QString();
}

} // namespace

HomePage::HomePage(MainWindow *window, QWidget *parent)
    : Page(window, parent)
{
    setupUi();
}

void HomePage::setupUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    // 可滚动区域
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral(
        "QScrollArea { background-color: %1; border: none; }"
        "QScrollArea > QWidget > QWidget { background-color: %1; }")
        .arg(UiTheme::kWindowBg.name()));
    scroll->viewport()->setAutoFillBackground(true);
    scroll->viewport()->setStyleSheet(QStringLiteral(
        "background-color: %1;").arg(UiTheme::kWindowBg.name()));

    auto *content = new QWidget(scroll);
    content->setAutoFillBackground(true);
    content->setStyleSheet(QStringLiteral(
        "background-color: %1;").arg(UiTheme::kWindowBg.name()));
    m_contentLayout = new QVBoxLayout(content);
    m_contentLayout->setContentsMargins(40, 32, 40, 40);
    m_contentLayout->setSpacing(20);

    // ---- 顶部标题 ----
    auto *header = new QWidget(content);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);

    auto *titleBox = new QVBoxLayout();
    titleBox->setSpacing(2);
    auto *title = UiTheme::createTitle(QStringLiteral("Weqi"), 30, header);
    titleBox->addWidget(title);
    m_subtitle = UiTheme::createMutedLabel(tr("现代国际象棋"), header);
    m_subtitle->setStyleSheet(QStringLiteral("color: %1; font-size: 13px;").arg(UiTheme::kMutedText.name()));
    titleBox->addWidget(m_subtitle);
    headerLayout->addLayout(titleBox);
    headerLayout->addStretch(1);
    m_contentLayout->addWidget(header);

    // ---- Continue Game 卡片（有未完成对局时显示）----
    m_continueCard = createContinueCard();
    m_contentLayout->addWidget(m_continueCard);

    // ---- 玩家资料卡片 ----
    m_profileCard = createProfileCard();
    m_contentLayout->addWidget(m_profileCard);

    // ---- Quick Play ----
    m_contentLayout->addWidget(createQuickPlayCard());

    // ---- 游戏模式 ----
    m_contentLayout->addWidget(createGameModesCard());

    // ---- 战绩 ----
    m_statsCard = createStatsCard();
    m_contentLayout->addWidget(m_statsCard);

    // ---- 最近对局 ----
    m_recentCard = createRecentGamesCard();
    m_contentLayout->addWidget(m_recentCard);

    // ---- 底部导航 ----
    m_contentLayout->addWidget(createFooter());

    m_contentLayout->addStretch(1);

    scroll->setWidget(content);
    rootLayout->addWidget(scroll);
}

bool HomePage::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            for (const ModeCard &mc : m_modeCards) {
                if (watched == mc.frame) {
                    if (mc.action)
                        mc.action();
                    return true;
                }
            }
        }
    }
    return Page::eventFilter(watched, event);
}

void HomePage::onShown()
{
    refresh();
    animateIn();
}

void HomePage::retranslateUi()
{
    // 静态文本
    m_subtitle->setText(tr("现代国际象棋"));
    m_quickPlayTitle->setText(tr("QUICK PLAY"));
    m_quickPlayBtn->setText(tr("▶  开始对局"));
    m_continueTitle->setText(tr("未完成对局"));
    m_continueDesc->setText(tr("有一盘进行中的对局，可以继续。"));
    m_continueBtn->setText(tr("继续对局"));
    m_settingsBtn->setText(tr("设置"));
    m_historyBtn->setText(tr("历史"));
    m_aboutBtn->setText(tr("关于"));

    // 游戏模式卡片
    const QStringList modeLabels = {
        tr("真人对战"), tr("人机对战"), tr("AI 对战"), tr("复盘")
    };
    const QStringList modeDescs = {
        tr("本地双人对弈"), tr("与 AI 对弈"), tr("AI 与 AI 对弈"), tr("查看历史对局")
    };
    for (int i = 0; i < m_modeLabels.size() && i < modeLabels.size(); ++i)
        m_modeLabels[i]->setText(modeLabels[i]);
    for (int i = 0; i < m_modeDescs.size() && i < modeDescs.size(); ++i)
        m_modeDescs[i]->setText(modeDescs[i]);

    // 动态卡片（资料/战绩/最近对局）通过重建刷新
    refresh();
}

void HomePage::refresh()
{
    // 刷新 Continue Game 卡片可见性
    const bool hasSaved = QFile::exists(DataPaths::currentGameFile());
    m_continueCard->setVisible(hasSaved);

    // 刷新玩家资料
    if (m_profileCard) {
        // 通过重建卡片刷新（简单可靠）
        m_contentLayout->removeWidget(m_profileCard);
        m_profileCard->deleteLater();
        m_profileCard = createProfileCard();
        m_contentLayout->insertWidget(2, m_profileCard);
    }

    // 刷新战绩
    m_window->stats()->refresh();
    if (m_statsCard) {
        m_contentLayout->removeWidget(m_statsCard);
        m_statsCard->deleteLater();
        m_statsCard = createStatsCard();
        m_contentLayout->insertWidget(5, m_statsCard);
    }

    // 刷新最近对局
    if (m_recentCard) {
        m_contentLayout->removeWidget(m_recentCard);
        m_recentCard->deleteLater();
        m_recentCard = createRecentGamesCard();
        m_contentLayout->insertWidget(6, m_recentCard);
    }
}

QWidget *HomePage::createProfileCard()
{
    auto *card = UiTheme::createCard(this);
    auto *layout = new QHBoxLayout(card);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(20);

    // 头像（圆形占位）
    auto *avatar = new QLabel(card);
    avatar->setFixedSize(64, 64);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setStyleSheet(QStringLiteral(
        "QLabel { background-color: %1; color: %2; border-radius: 32px;"
        " font-size: 28px; font-weight: bold; }")
        .arg(UiTheme::kAccent.name()).arg(UiTheme::kTitleColor.name()));
    const QString name = m_window->profile()->playerName();
    avatar->setText(name.isEmpty() ? QStringLiteral("?") : name.left(1).toUpper());
    layout->addWidget(avatar);

    // 名称 + Rating
    auto *infoBox = new QVBoxLayout();
    infoBox->setSpacing(4);
    auto *nameLabel = UiTheme::createTitle(name, 20, card);
    infoBox->addWidget(nameLabel);

    auto *ratingLabel = UiTheme::createMutedLabel(
        tr("Rating  %1").arg(m_window->profile()->rating()), card);
    ratingLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 14px;").arg(UiTheme::kMutedText.name()));
    infoBox->addWidget(ratingLabel);
    layout->addLayout(infoBox);

    layout->addStretch(1);

    // 编辑资料按钮
    auto *editBtn = UiTheme::createGhostButton(tr("编辑资料"), card);
    connect(editBtn, &QPushButton::clicked, this, [this]() {
        m_window->showSettings();
    });
    layout->addWidget(editBtn);

    return card;
}

QWidget *HomePage::createQuickPlayCard()
{
    auto *card = UiTheme::createCard(this);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);

    m_quickPlayTitle = UiTheme::createSectionLabel(tr("QUICK PLAY"), card);
    layout->addWidget(m_quickPlayTitle);

    m_quickPlayBtn = UiTheme::createPrimaryButton(tr("▶  开始对局"), card);
    m_quickPlayBtn->setMinimumHeight(56);
    m_quickPlayBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: %1; color: %2; border: none;"
        " border-radius: 14px; padding: 16px 24px; font-size: 18px; font-weight: bold; }"
        "QPushButton:hover { background-color: %3; }")
        .arg(UiTheme::kAccent.name()).arg(UiTheme::kTitleColor.name())
        .arg(QColor(0x5A, 0xC8, 0x5E).name()));
    connect(m_quickPlayBtn, &QPushButton::clicked, this, [this]() {
        m_window->showNewGame();
    });
    layout->addWidget(m_quickPlayBtn);

    return card;
}

QWidget *HomePage::createGameModesCard()
{
    auto *card = UiTheme::createCard(this);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);

    auto *title = UiTheme::createSectionLabel(tr("游戏模式"), card);
    layout->addWidget(title);

    // 模式按钮网格
    auto *grid = new QGridLayout();
    grid->setSpacing(12);

    struct ModeEntry {
        QString label;
        QString desc;
        std::function<void()> action;
    };

    const QVector<ModeEntry> modes = {
        { tr("真人对战"), tr("本地双人对弈"),
          [this]() { m_window->startGame(GameMode::HumanVsHuman, QStringLiteral("Human"),
                                          m_window->profile()->playerName(), QStringLiteral("对手")); } },
        { tr("人机对战"), tr("与 AI 对弈"),
          [this]() { m_window->showAIOpponent(); } },
        { tr("AI 对战"), tr("AI 与 AI 对弈"),
          [this]() { m_window->showAIVsAI(); } },
        { tr("复盘"), tr("查看历史对局"),
          [this]() { m_window->showHistory(); } },
    };

    int idx = 0;
    for (const ModeEntry &m : modes) {
        // 用 QFrame 作为可点击卡片（QPushButton 内部放子 widget 不可靠，
        // 按钮会覆盖子 widget 的绘制）。通过事件过滤器处理点击。
        auto *modeCard = new QFrame(card);
        modeCard->setMinimumHeight(72);
        modeCard->setCursor(Qt::PointingHandCursor);
        modeCard->setStyleSheet(QStringLiteral(
            "QFrame { background-color: %1; border: none; border-radius: 14px; }"
            "QFrame:hover { background-color: %2; }")
            .arg(UiTheme::kPanelBg.name()).arg(UiTheme::kAccentSoft.name()));

        auto *btnLayout = new QVBoxLayout(modeCard);
        btnLayout->setContentsMargins(16, 12, 16, 12);
        btnLayout->setSpacing(2);
        auto *label = new QLabel(m.label, modeCard);
        label->setStyleSheet(QStringLiteral("color: %1; font-size: 16px; font-weight: bold;").arg(UiTheme::kTitleColor.name()));
        label->setAttribute(Qt::WA_TransparentForMouseEvents);
        btnLayout->addWidget(label);
        auto *desc = new QLabel(m.desc, modeCard);
        desc->setStyleSheet(QStringLiteral("color: %1; font-size: 12px;").arg(UiTheme::kMutedText.name()));
        desc->setAttribute(Qt::WA_TransparentForMouseEvents);
        btnLayout->addWidget(desc);

        m_modeLabels.append(label);
        m_modeDescs.append(desc);

        // 点击处理：安装事件过滤器
        modeCard->installEventFilter(this);
        m_modeCards.append({ modeCard, m.action });

        grid->addWidget(modeCard, idx / 2, idx % 2);
        ++idx;
    }

    layout->addLayout(grid);
    return card;
}

QWidget *HomePage::createStatsCard()
{
    auto *card = UiTheme::createCard(this);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);

    auto *title = UiTheme::createSectionLabel(tr("你的战绩"), card);
    layout->addWidget(title);

    const PlayerStats &s = m_window->stats()->stats();

    // 统计网格
    auto *grid = new QGridLayout();
    grid->setSpacing(12);

    struct StatEntry {
        QString label;
        QString value;
    };

    const QVector<StatEntry> stats = {
        { tr("对局数"), QString::number(s.gamesPlayed) },
        { tr("胜率"), QStringLiteral("%1%").arg(s.winRate, 0, 'f', 0) },
        { tr("胜"), QString::number(s.wins) },
        { tr("和"), QString::number(s.draws) },
        { tr("负"), QString::number(s.losses) },
        { tr("最高连胜"), QString::number(s.bestWinStreak) },
        { tr("当前连胜"), QString::number(s.currentStreak) },
        { tr("最佳 Rating"), QString::number(m_window->profile()->bestRating()) },
    };

    int idx = 0;
    for (const StatEntry &st : stats) {
        auto *cell = new QFrame(card);
        cell->setStyleSheet(QStringLiteral(
            "QFrame { background-color: %1; border-radius: 12px; }").arg(UiTheme::kPanelBg.name()));
        auto *cellLayout = new QVBoxLayout(cell);
        cellLayout->setContentsMargins(16, 12, 16, 12);
        cellLayout->setSpacing(2);
        auto *value = new QLabel(st.value, cell);
        value->setStyleSheet(QStringLiteral("color: %1; font-size: 20px; font-weight: bold;").arg(UiTheme::kTitleColor.name()));
        cellLayout->addWidget(value);
        auto *label = new QLabel(st.label, cell);
        label->setStyleSheet(QStringLiteral("color: %1; font-size: 12px;").arg(UiTheme::kMutedText.name()));
        cellLayout->addWidget(label);
        grid->addWidget(cell, idx / 4, idx % 4);
        ++idx;
    }

    layout->addLayout(grid);
    return card;
}

QWidget *HomePage::createRecentGamesCard()
{
    auto *card = UiTheme::createCard(this);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);

    auto *title = UiTheme::createSectionLabel(tr("最近对局"), card);
    layout->addWidget(title);

    const QVector<GameRecord> recent = m_window->history()->recent(5);
    if (recent.isEmpty()) {
        auto *empty = UiTheme::createMutedLabel(tr("还没有对局记录，开始你的第一盘棋吧。"), card);
        layout->addWidget(empty);
        return card;
    }

    for (const GameRecord &rec : recent) {
        auto *row = new QFrame(card);
        row->setStyleSheet(QStringLiteral(
            "QFrame { background-color: %1; border-radius: 12px; }").arg(UiTheme::kPanelBg.name()));
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(16, 12, 16, 12);
        rowLayout->setSpacing(12);

        // 日期
        auto *date = new QLabel(rec.date.toString(QStringLiteral("MM-dd HH:mm")), row);
        date->setStyleSheet(QStringLiteral("color: %1; font-size: 12px;").arg(UiTheme::kMutedText.name()));
        rowLayout->addWidget(date);

        // 对手 + 类型
        auto *info = new QLabel(QStringLiteral("%1 · %2").arg(rec.opponent, modeText(rec.mode)), row);
        info->setStyleSheet(QStringLiteral("color: %1; font-size: 13px;").arg(UiTheme::kTitleColor.name()));
        rowLayout->addWidget(info, 1);

        // 结果（AI vs AI 不计入玩家战绩，仅显示 resultText 作汇报，不标红）
        QColor resultColor = UiTheme::kMutedText;
        if (rec.outcome == GameOutcome::Win)
            resultColor = UiTheme::kWin;
        else if (rec.outcome == GameOutcome::Loss)
            resultColor = UiTheme::kDanger;
        const QString resultLabel = (rec.mode == GameMode::AIVsAI)
                                        ? rec.resultText
                                        : QStringLiteral("%1 %2").arg(outcomeText(rec.outcome), rec.resultText);
        auto *result = new QLabel(resultLabel, row);
        result->setStyleSheet(QStringLiteral("color: %1; font-size: 13px; font-weight: bold;").arg(resultColor.name()));
        rowLayout->addWidget(result);

        // Rating 变化
        if (rec.ratingChange != 0) {
            const QString sign = rec.ratingChange > 0 ? QStringLiteral("+") : QString();
            auto *rating = new QLabel(QStringLiteral("%1%2").arg(sign).arg(rec.ratingChange), row);
            rating->setStyleSheet(QStringLiteral("color: %1; font-size: 13px; font-weight: bold;")
                                      .arg(rec.ratingChange > 0 ? UiTheme::kWin.name() : UiTheme::kDanger.name()));
            rowLayout->addWidget(rating);
        }

        layout->addWidget(row);
    }

    // 查看全部
    auto *viewAll = UiTheme::createGhostButton(tr("查看全部对局"), card);
    connect(viewAll, &QPushButton::clicked, this, [this]() {
        m_window->showHistory();
    });
    layout->addWidget(viewAll);

    return card;
}

QWidget *HomePage::createContinueCard()
{
    auto *card = UiTheme::createCard(this);
    card->setStyleSheet(UiTheme::cardStyle() + QStringLiteral(
        " QFrame { border: 1px solid %1; }").arg(UiTheme::kAccent.name()));
    auto *layout = new QHBoxLayout(card);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(16);

    auto *infoBox = new QVBoxLayout();
    infoBox->setSpacing(2);
    m_continueTitle = UiTheme::createSectionLabel(tr("未完成对局"), card);
    infoBox->addWidget(m_continueTitle);
    m_continueDesc = UiTheme::createMutedLabel(tr("有一盘进行中的对局，可以继续。"), card);
    infoBox->addWidget(m_continueDesc);
    layout->addLayout(infoBox);

    layout->addStretch(1);

    m_continueBtn = UiTheme::createPrimaryButton(tr("继续对局"), card);
    connect(m_continueBtn, &QPushButton::clicked, this, [this]() {
        m_window->continueGame();
    });
    layout->addWidget(m_continueBtn);

    return card;
}

QWidget *HomePage::createFooter()
{
    auto *footer = new QWidget(this);
    auto *layout = new QHBoxLayout(footer);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    m_settingsBtn = UiTheme::createGhostButton(tr("设置"), footer);
    connect(m_settingsBtn, &QPushButton::clicked, this, [this]() {
        m_window->showSettings();
    });
    layout->addWidget(m_settingsBtn);

    m_historyBtn = UiTheme::createGhostButton(tr("历史"), footer);
    connect(m_historyBtn, &QPushButton::clicked, this, [this]() {
        m_window->showHistory();
    });
    layout->addWidget(m_historyBtn);

    m_aboutBtn = UiTheme::createGhostButton(tr("关于"), footer);
    connect(m_aboutBtn, &QPushButton::clicked, this, [this]() {
        m_window->showAbout();
    });
    layout->addWidget(m_aboutBtn);

    layout->addStretch(1);
    return footer;
}

void HomePage::animateIn()
{
    // 首页淡入动画。
    // 注意：QGraphicsOpacityEffect 会强制 widget 离屏渲染，在 Wayland/xcb 下
    // 易导致整页黑块与卡顿。为保证 UI 稳定，这里暂不启用透明度动画，
    // 后续可在支持良好的平台上再开启。
    Q_UNUSED(m_profileCard);
    Q_UNUSED(m_statsCard);
    Q_UNUSED(m_recentCard);
}
