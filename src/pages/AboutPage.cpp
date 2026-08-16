#include "AboutPage.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

#include "MainWindow.h"
#include "UiTheme.h"

AboutPage::AboutPage(MainWindow *window, QWidget *parent)
    : Page(window, parent)
{
    setupUi();
}

void AboutPage::setupUi()
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

    auto *title = UiTheme::createTitle(QStringLiteral("关于"), 24, header);
    headerLayout->addWidget(title);
    headerLayout->addStretch(1);
    rootLayout->addWidget(header);

    // 内容卡片
    auto *card = UiTheme::createCard(this);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(32, 32, 32, 32);
    layout->setSpacing(16);

    auto *appName = UiTheme::createTitle(QStringLiteral("Weqi"), 32, card);
    appName->setAlignment(Qt::AlignCenter);
    layout->addWidget(appName);

    auto *tagline = UiTheme::createMutedLabel(QStringLiteral("现代国际象棋"), card);
    tagline->setAlignment(Qt::AlignCenter);
    layout->addWidget(tagline);

    auto *version = UiTheme::createMutedLabel(QStringLiteral("版本 0.1.0"), card);
    version->setAlignment(Qt::AlignCenter);
    layout->addWidget(version);

    auto *desc = UiTheme::createMutedLabel(
        QStringLiteral("Weqi 是一款开源的桌面国际象棋应用，"
                       "所有棋规均在本地 C++ 引擎中实现，"
                       "支持真人对战、人机对战、AI 对战与复盘。"),
        card);
    desc->setWordWrap(true);
    desc->setAlignment(Qt::AlignCenter);
    layout->addWidget(desc);

    auto *tech = UiTheme::createMutedLabel(
        QStringLiteral("技术栈：C++17 · Qt 6 · CMake"), card);
    tech->setAlignment(Qt::AlignCenter);
    layout->addWidget(tech);

    layout->addStretch(1);

    rootLayout->addWidget(card);
    rootLayout->addStretch(1);
}
