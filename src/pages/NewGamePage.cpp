#include "NewGamePage.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>

#include "MainWindow.h"
#include "UiTheme.h"
#include "ProfileManager.h"
#include "AIProviderManager.h"
#include "GameRecord.h"

NewGamePage::NewGamePage(MainWindow *window, QWidget *parent)
    : Page(window, parent)
{
    setupUi();
}

void NewGamePage::setupUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(40, 32, 40, 40);
    rootLayout->setSpacing(20);

    // 顶部标题栏
    auto *header = new QWidget(this);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(14);

    m_backBtn = UiTheme::createGhostButton(tr("← 首页"), header);
    connect(m_backBtn, &QPushButton::clicked, this, [this]() {
        m_window->showHome();
    });
    headerLayout->addWidget(m_backBtn);

    m_headerTitle = UiTheme::createTitle(tr("新游戏"), 24, header);
    headerLayout->addWidget(m_headerTitle);
    headerLayout->addStretch(1);
    rootLayout->addWidget(header);

    // 设置卡片
    auto *card = UiTheme::createCard(this);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(32, 32, 32, 32);
    layout->setSpacing(20);

    // 对手选择
    m_opponentLabel = UiTheme::createSectionLabel(tr("对手"), card);
    layout->addWidget(m_opponentLabel);

    m_opponentCombo = new QComboBox(card);
    m_opponentCombo->addItem(tr("真人（本地双人）"), static_cast<int>(GameMode::HumanVsHuman));
    m_opponentCombo->addItem(tr("AI 对手"), static_cast<int>(GameMode::HumanVsAI));
    m_opponentCombo->addItem(tr("AI vs AI"), static_cast<int>(GameMode::AIVsAI));
    m_opponentCombo->setStyleSheet(UiTheme::comboStyle());
    layout->addWidget(m_opponentCombo);

    // 白方名称
    m_whiteLabel = UiTheme::createSectionLabel(tr("白方名称"), card);
    layout->addWidget(m_whiteLabel);
    m_whiteNameEdit = new QLineEdit(card);
    m_whiteNameEdit->setStyleSheet(UiTheme::inputStyle());
    layout->addWidget(m_whiteNameEdit);

    // 黑方名称
    m_blackLabel = UiTheme::createSectionLabel(tr("黑方名称"), card);
    layout->addWidget(m_blackLabel);
    m_blackNameEdit = new QLineEdit(card);
    m_blackNameEdit->setStyleSheet(UiTheme::inputStyle());
    layout->addWidget(m_blackNameEdit);

    // AI 配置入口（选择 AI 对手时显示）
    m_aiHint = UiTheme::createMutedLabel(
        tr("选择 AI 对手后，可在下一步选择 AI 提供商与模型。"), card);
    m_aiHint->setWordWrap(true);
    layout->addWidget(m_aiHint);

    layout->addStretch(1);

    // 开始按钮
    m_startBtn = UiTheme::createPrimaryButton(tr("开始对局"), card);
    m_startBtn->setMinimumHeight(52);
    connect(m_startBtn, &QPushButton::clicked, this, &NewGamePage::startGame);
    layout->addWidget(m_startBtn);

    rootLayout->addWidget(card);
    rootLayout->addStretch(1);
}

void NewGamePage::retranslateUi()
{
    m_backBtn->setText(tr("← 首页"));
    m_headerTitle->setText(tr("新游戏"));
    m_opponentLabel->setText(tr("对手"));
    m_opponentCombo->setItemText(0, tr("真人（本地双人）"));
    m_opponentCombo->setItemText(1, tr("AI 对手"));
    m_opponentCombo->setItemText(2, tr("AI vs AI"));
    m_whiteLabel->setText(tr("白方名称"));
    m_blackLabel->setText(tr("黑方名称"));
    m_aiHint->setText(tr("选择 AI 对手后，可在下一步选择 AI 提供商与模型。"));
    m_startBtn->setText(tr("开始对局"));
}

void NewGamePage::onShown()
{
    // 默认白方为玩家名称
    if (m_whiteNameEdit->text().isEmpty())
        m_whiteNameEdit->setText(m_window->profile()->playerName());
    if (m_blackNameEdit->text().isEmpty())
        m_blackNameEdit->setText(QStringLiteral("对手"));
}

void NewGamePage::startGame()
{
    const GameMode mode = static_cast<GameMode>(m_opponentCombo->currentData().toInt());
    const QString whiteName = m_whiteNameEdit->text().trimmed().isEmpty()
                                  ? m_window->profile()->playerName()
                                  : m_whiteNameEdit->text().trimmed();
    const QString blackName = m_blackNameEdit->text().trimmed().isEmpty()
                                  ? QStringLiteral("对手")
                                  : m_blackNameEdit->text().trimmed();

    if (mode == GameMode::HumanVsAI) {
        // 进入 AI 对手选择页
        m_window->showAIOpponent();
        return;
    }

    if (mode == GameMode::AIVsAI) {
        // 进入 AI vs AI 设置页
        m_window->showAIVsAI();
        return;
    }

    m_window->startGame(mode, QStringLiteral("Human"), whiteName, blackName);
}
