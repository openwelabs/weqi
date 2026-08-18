#include "AIVsAIPage.h"

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
#include "GameRecord.h"

AIVsAIPage::AIVsAIPage(MainWindow *window, QWidget *parent)
    : Page(window, parent)
{
    setupUi();
}

void AIVsAIPage::setupUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(40, 32, 40, 40);
    rootLayout->setSpacing(20);

    // 顶部标题栏
    auto *header = new QWidget(this);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(14);

    m_backBtn = UiTheme::createGhostButton(tr("← 返回"), header);
    connect(m_backBtn, &QPushButton::clicked, this, [this]() {
        m_window->showNewGame();
    });
    headerLayout->addWidget(m_backBtn);

    m_headerTitle = UiTheme::createTitle(tr("AI vs AI"), 24, header);
    headerLayout->addWidget(m_headerTitle);
    headerLayout->addStretch(1);
    rootLayout->addWidget(header);

    // 说明
    m_intro = UiTheme::createMutedLabel(
        tr("选择两个 AI 进行对弈。两个 AI 将轮流走子，由 C++ 控制整个对局。"), this);
    m_intro->setWordWrap(true);
    rootLayout->addWidget(m_intro);

    // 主卡片
    auto *card = UiTheme::createCard(this);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);

    // ---- 白方 AI ----
    m_whiteTitle = UiTheme::createSectionLabel(tr("白方 AI"), card);
    layout->addWidget(m_whiteTitle);

    m_whiteList = new QListWidget(card);
    m_whiteList->setStyleSheet(QStringLiteral(
        "QListWidget { background-color: %1; border: none; border-radius: 12px;"
        " color: %2; font-size: 15px; padding: 8px; }"
        "QListWidget::item { padding: 14px 12px; border-radius: 10px; }"
        "QListWidget::item:selected { background-color: %3; }")
        .arg(UiTheme::kPanelBg.name()).arg(UiTheme::kTitleColor.name()).arg(UiTheme::kAccentSoft.name()));
    m_whiteList->setFixedHeight(120);
    layout->addWidget(m_whiteList);

    // ---- 黑方 AI ----
    m_blackTitle = UiTheme::createSectionLabel(tr("黑方 AI"), card);
    layout->addWidget(m_blackTitle);

    m_blackList = new QListWidget(card);
    m_blackList->setStyleSheet(QStringLiteral(
        "QListWidget { background-color: %1; border: none; border-radius: 12px;"
        " color: %2; font-size: 15px; padding: 8px; }"
        "QListWidget::item { padding: 14px 12px; border-radius: 10px; }"
        "QListWidget::item:selected { background-color: %3; }")
        .arg(UiTheme::kPanelBg.name()).arg(UiTheme::kTitleColor.name()).arg(UiTheme::kAccentSoft.name()));
    m_blackList->setFixedHeight(120);
    layout->addWidget(m_blackList);

    // 无配置提示（默认隐藏）
    m_noConfigLabel = UiTheme::createMutedLabel(
        tr("尚未配置任何 AI Provider。请先在设置中配置 AI。"), card);
    m_noConfigLabel->setWordWrap(true);
    m_noConfigLabel->hide();
    layout->addWidget(m_noConfigLabel);

    // 配置 AI 按钮（默认隐藏）
    m_configureBtn = UiTheme::createSecondaryButton(tr("配置 AI"), card);
    m_configureBtn->setMinimumHeight(44);
    connect(m_configureBtn, &QPushButton::clicked, this, [this]() {
        m_window->showSettings();
    });
    m_configureBtn->hide();
    layout->addWidget(m_configureBtn);

    // 开始按钮
    m_startBtn = UiTheme::createPrimaryButton(tr("开始对局"), card);
    m_startBtn->setMinimumHeight(52);
    connect(m_startBtn, &QPushButton::clicked, this, &AIVsAIPage::startGame);
    layout->addWidget(m_startBtn);

    rootLayout->addWidget(card);
    rootLayout->addStretch(1);
}

void AIVsAIPage::retranslateUi()
{
    m_backBtn->setText(tr("← 返回"));
    m_headerTitle->setText(tr("AI vs AI"));
    m_intro->setText(tr("选择两个 AI 进行对弈。两个 AI 将轮流走子，由 C++ 控制整个对局。"));
    m_whiteTitle->setText(tr("白方 AI"));
    m_blackTitle->setText(tr("黑方 AI"));
    m_noConfigLabel->setText(tr("尚未配置任何 AI Provider。请先在设置中配置 AI。"));
    m_configureBtn->setText(tr("配置 AI"));
    m_startBtn->setText(tr("开始对局"));
}

void AIVsAIPage::refreshProviderLists()
{
    m_whiteList->clear();
    m_blackList->clear();

    const QVector<AIProvider> &providers = m_window->aiProviders()->providers();
    for (const AIProvider &p : providers) {
        const QString label = QStringLiteral("%1  ·  %2").arg(p.name, p.model);
        auto *whiteItem = new QListWidgetItem(label, m_whiteList);
        whiteItem->setData(Qt::UserRole, p.id);
        m_whiteList->addItem(whiteItem);

        auto *blackItem = new QListWidgetItem(label, m_blackList);
        blackItem->setData(Qt::UserRole, p.id);
        m_blackList->addItem(blackItem);
    }

    const bool hasProviders = !providers.isEmpty();
    m_whiteList->setVisible(hasProviders);
    m_blackList->setVisible(hasProviders);
    m_noConfigLabel->setVisible(!hasProviders);
    m_configureBtn->setVisible(!hasProviders);
    m_startBtn->setEnabled(hasProviders);

    if (hasProviders) {
        m_whiteList->setCurrentRow(0);
        m_blackList->setCurrentRow(0);
    }
}

void AIVsAIPage::onShown()
{
    refreshProviderLists();
}

QString AIVsAIPage::currentWhiteProviderId() const
{
    QListWidgetItem *item = m_whiteList->currentItem();
    if (!item)
        return QString();
    return item->data(Qt::UserRole).toString();
}

QString AIVsAIPage::currentBlackProviderId() const
{
    QListWidgetItem *item = m_blackList->currentItem();
    if (!item)
        return QString();
    return item->data(Qt::UserRole).toString();
}

void AIVsAIPage::startGame()
{
    const QString whiteProviderId = currentWhiteProviderId();
    const QString blackProviderId = currentBlackProviderId();
    if (whiteProviderId.isEmpty() || blackProviderId.isEmpty())
        return;

    const AIProvider *whiteProvider = m_window->aiProviders()->providerById(whiteProviderId);
    const AIProvider *blackProvider = m_window->aiProviders()->providerById(blackProviderId);
    if (!whiteProvider || !blackProvider)
        return;

    const QString whiteName = whiteProvider->name;
    const QString blackName = blackProvider->name;

    m_window->startAIGame(GameMode::AIVsAI, QStringLiteral("AI vs AI"),
                          whiteName, blackName,
                          true, true, whiteProviderId, blackProviderId);
}
