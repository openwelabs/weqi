#include "AIOpponentPage.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>

#include "MainWindow.h"
#include "UiTheme.h"
#include "AIProviderManager.h"
#include "ProfileManager.h"
#include "GameRecord.h"

AIOpponentPage::AIOpponentPage(MainWindow *window, QWidget *parent)
    : Page(window, parent)
{
    setupUi();
}

void AIOpponentPage::setupUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(40, 32, 40, 40);
    rootLayout->setSpacing(20);

    // 顶部标题栏
    auto *header = new QWidget(this);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(14);

    auto *backBtn = UiTheme::createGhostButton(QStringLiteral("← 返回"), header);
    connect(backBtn, &QPushButton::clicked, this, [this]() {
        m_window->showNewGame();
    });
    headerLayout->addWidget(backBtn);

    auto *title = UiTheme::createTitle(QStringLiteral("选择 AI 对手"), 24, header);
    headerLayout->addWidget(title);
    headerLayout->addStretch(1);
    rootLayout->addWidget(header);

    // 说明
    auto *intro = UiTheme::createMutedLabel(
        QStringLiteral("选择要与你对弈的 AI。当前为界面预览，AI 引擎将在后续版本接入。"), this);
    intro->setWordWrap(true);
    rootLayout->addWidget(intro);

    // AI 列表卡片
    auto *card = UiTheme::createCard(this);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);

    m_aiList = new QListWidget(card);
    m_aiList->setStyleSheet(QStringLiteral(
        "QListWidget { background-color: %1; border: none; border-radius: 12px;"
        " color: %2; font-size: 15px; padding: 8px; }"
        "QListWidget::item { padding: 14px 12px; border-radius: 10px; }"
        "QListWidget::item:selected { background-color: %3; }")
        .arg(UiTheme::kPanelBg.name()).arg(UiTheme::kTitleColor.name()).arg(UiTheme::kAccentSoft.name()));

    // 内置 AI
    auto *webuItem = new QListWidgetItem(QStringLiteral("Webu  ·  内置 AI"), m_aiList);
    webuItem->setData(Qt::UserRole, QStringLiteral("Webu"));
    m_aiList->addItem(webuItem);

    // 自定义 AI（来自配置的 Providers）
    const QVector<AIProvider> &providers = m_window->aiProviders()->providers();
    for (const AIProvider &p : providers) {
        auto *item = new QListWidgetItem(QStringLiteral("%1  ·  %2").arg(p.name, p.model), m_aiList);
        item->setData(Qt::UserRole, p.name);
        m_aiList->addItem(item);
    }

    // 未来扩展提示
    auto *future = UiTheme::createMutedLabel(
        QStringLiteral("未来将支持：OpenAI 兼容接口、本地模型、第三方 AI。"), card);
    future->setWordWrap(true);
    layout->addWidget(future);

    layout->addWidget(m_aiList);

    // 描述
    m_descLabel = UiTheme::createMutedLabel(QStringLiteral("选择一个 AI 对手开始对局。"), card);
    m_descLabel->setWordWrap(true);
    layout->addWidget(m_descLabel);

    // 开始按钮
    auto *startBtn = UiTheme::createPrimaryButton(QStringLiteral("开始对局"), card);
    startBtn->setMinimumHeight(52);
    connect(startBtn, &QPushButton::clicked, this, &AIOpponentPage::startGame);
    layout->addWidget(startBtn);

    rootLayout->addWidget(card);
    rootLayout->addStretch(1);
}

void AIOpponentPage::onShown()
{
    if (m_aiList->count() > 0)
        m_aiList->setCurrentRow(0);
}

void AIOpponentPage::startGame()
{
    QListWidgetItem *item = m_aiList->currentItem();
    if (!item)
        return;

    const QString aiName = item->data(Qt::UserRole).toString();
    const QString playerName = m_window->profile()->playerName();
    m_window->startGame(GameMode::HumanVsAI, aiName, playerName, aiName);
}
