#include "AIOpponentPage.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QComboBox>
#include <QRandomGenerator>

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

    m_backBtn = UiTheme::createGhostButton(tr("← 返回"), header);
    connect(m_backBtn, &QPushButton::clicked, this, [this]() {
        m_window->showNewGame();
    });
    headerLayout->addWidget(m_backBtn);

    m_headerTitle = UiTheme::createTitle(tr("选择 AI 对手"), 24, header);
    headerLayout->addWidget(m_headerTitle);
    headerLayout->addStretch(1);
    rootLayout->addWidget(header);

    // 说明
    m_intro = UiTheme::createMutedLabel(
        tr("选择要与你对弈的 AI，以及 AI 执子方。"), this);
    m_intro->setWordWrap(true);
    rootLayout->addWidget(m_intro);

    // 主卡片
    auto *card = UiTheme::createCard(this);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);

    // ---- AI Provider 列表 ----
    m_aiTitle = UiTheme::createSectionLabel(tr("选择 AI"), card);
    layout->addWidget(m_aiTitle);

    m_aiList = new QListWidget(card);
    m_aiList->setStyleSheet(QStringLiteral(
        "QListWidget { background-color: %1; border: none; border-radius: 12px;"
        " color: %2; font-size: 15px; padding: 8px; }"
        "QListWidget::item { padding: 14px 12px; border-radius: 10px; }"
        "QListWidget::item:selected { background-color: %3; }")
        .arg(UiTheme::kPanelBg.name()).arg(UiTheme::kTitleColor.name()).arg(UiTheme::kAccentSoft.name()));
    layout->addWidget(m_aiList);

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

    // ---- 执子方选择 ----
    m_sideTitle = UiTheme::createSectionLabel(tr("AI 执子方"), card);
    layout->addWidget(m_sideTitle);

    m_sideCombo = new QComboBox(card);
    m_sideCombo->addItem(tr("白方（AI 执白）"), QStringLiteral("white"));
    m_sideCombo->addItem(tr("黑方（AI 执黑）"), QStringLiteral("black"));
    m_sideCombo->addItem(tr("随机"), QStringLiteral("random"));
    m_sideCombo->setStyleSheet(UiTheme::comboStyle());
    m_sideCombo->setMinimumHeight(44);
    layout->addWidget(m_sideCombo);

    // 描述
    m_descLabel = UiTheme::createMutedLabel(tr("选择一个 AI 对手开始对局。"), card);
    m_descLabel->setWordWrap(true);
    layout->addWidget(m_descLabel);

    // 开始按钮
    m_startBtn = UiTheme::createPrimaryButton(tr("开始对局"), card);
    m_startBtn->setMinimumHeight(52);
    connect(m_startBtn, &QPushButton::clicked, this, &AIOpponentPage::startGame);
    layout->addWidget(m_startBtn);

    rootLayout->addWidget(card);
    rootLayout->addStretch(1);
}

void AIOpponentPage::retranslateUi()
{
    m_backBtn->setText(tr("← 返回"));
    m_headerTitle->setText(tr("选择 AI 对手"));
    m_intro->setText(tr("选择要与你对弈的 AI，以及 AI 执子方。"));
    m_aiTitle->setText(tr("选择 AI"));
    m_noConfigLabel->setText(tr("尚未配置任何 AI Provider。请先在设置中配置 AI。"));
    m_configureBtn->setText(tr("配置 AI"));
    m_sideTitle->setText(tr("AI 执子方"));
    m_sideCombo->setItemText(0, tr("白方（AI 执白）"));
    m_sideCombo->setItemText(1, tr("黑方（AI 执黑）"));
    m_sideCombo->setItemText(2, tr("随机"));
    m_descLabel->setText(tr("选择一个 AI 对手开始对局。"));
    m_startBtn->setText(tr("开始对局"));
}

void AIOpponentPage::refreshProviderList()
{
    m_aiList->clear();

    const QVector<AIProvider> &providers = m_window->aiProviders()->providers();
    for (const AIProvider &p : providers) {
        auto *item = new QListWidgetItem(
            QStringLiteral("%1  ·  %2").arg(p.name, p.model), m_aiList);
        item->setData(Qt::UserRole, p.id);
        m_aiList->addItem(item);
    }

    const bool hasProviders = !providers.isEmpty();
    m_aiList->setVisible(hasProviders);
    m_noConfigLabel->setVisible(!hasProviders);
    m_configureBtn->setVisible(!hasProviders);
    m_startBtn->setEnabled(hasProviders);

    if (hasProviders)
        m_aiList->setCurrentRow(0);
}

void AIOpponentPage::onShown()
{
    refreshProviderList();
}

QString AIOpponentPage::currentProviderId() const
{
    QListWidgetItem *item = m_aiList->currentItem();
    if (!item)
        return QString();
    return item->data(Qt::UserRole).toString();
}

void AIOpponentPage::startGame()
{
    const QString providerId = currentProviderId();
    if (providerId.isEmpty())
        return;

    const AIProvider *provider = m_window->aiProviders()->providerById(providerId);
    if (!provider)
        return;

    const QString playerName = m_window->profile()->playerName();
    const QString aiName = provider->name;

    // 确定 AI 执子方
    const QString side = m_sideCombo->currentData().toString();
    bool aiIsWhite = false;
    if (side == QStringLiteral("white")) {
        aiIsWhite = true;
    } else if (side == QStringLiteral("black")) {
        aiIsWhite = false;
    } else { // random
        aiIsWhite = (QRandomGenerator::global()->bounded(2) == 0);
    }

    if (aiIsWhite) {
        m_window->startAIGame(GameMode::HumanVsAI, aiName, aiName, playerName,
                              true, false, providerId, QString());
    } else {
        m_window->startAIGame(GameMode::HumanVsAI, aiName, playerName, aiName,
                              false, true, QString(), providerId);
    }
}
