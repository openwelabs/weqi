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

    auto *backBtn = UiTheme::createGhostButton(QStringLiteral("← 首页"), header);
    connect(backBtn, &QPushButton::clicked, this, [this]() {
        m_window->showHome();
    });
    headerLayout->addWidget(backBtn);

    auto *title = UiTheme::createTitle(QStringLiteral("新游戏"), 24, header);
    headerLayout->addWidget(title);
    headerLayout->addStretch(1);
    rootLayout->addWidget(header);

    // 设置卡片
    auto *card = UiTheme::createCard(this);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(32, 32, 32, 32);
    layout->setSpacing(20);

    // 对手选择
    auto *opponentLabel = UiTheme::createSectionLabel(QStringLiteral("对手"), card);
    layout->addWidget(opponentLabel);

    m_opponentCombo = new QComboBox(card);
    m_opponentCombo->addItem(QStringLiteral("真人（本地双人）"), static_cast<int>(GameMode::HumanVsHuman));
    m_opponentCombo->addItem(QStringLiteral("AI 对手"), static_cast<int>(GameMode::HumanVsAI));
    m_opponentCombo->setStyleSheet(UiTheme::comboStyle());
    layout->addWidget(m_opponentCombo);

    // 白方名称
    auto *whiteLabel = UiTheme::createSectionLabel(QStringLiteral("白方名称"), card);
    layout->addWidget(whiteLabel);
    m_whiteNameEdit = new QLineEdit(card);
    m_whiteNameEdit->setStyleSheet(UiTheme::inputStyle());
    layout->addWidget(m_whiteNameEdit);

    // 黑方名称
    auto *blackLabel = UiTheme::createSectionLabel(QStringLiteral("黑方名称"), card);
    layout->addWidget(blackLabel);
    m_blackNameEdit = new QLineEdit(card);
    m_blackNameEdit->setStyleSheet(UiTheme::inputStyle());
    layout->addWidget(m_blackNameEdit);

    // AI 配置入口（选择 AI 对手时显示）
    auto *aiHint = UiTheme::createMutedLabel(
        QStringLiteral("选择 AI 对手后，可在下一步选择 AI 提供商与模型。"), card);
    aiHint->setWordWrap(true);
    layout->addWidget(aiHint);

    layout->addStretch(1);

    // 开始按钮
    auto *startBtn = UiTheme::createPrimaryButton(QStringLiteral("开始对局"), card);
    startBtn->setMinimumHeight(52);
    connect(startBtn, &QPushButton::clicked, this, &NewGamePage::startGame);
    layout->addWidget(startBtn);

    rootLayout->addWidget(card);
    rootLayout->addStretch(1);
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

    m_window->startGame(mode, QStringLiteral("Human"), whiteName, blackName);
}
