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

    m_backBtn = UiTheme::createGhostButton(tr("← 首页"), header);
    connect(m_backBtn, &QPushButton::clicked, this, [this]() {
        m_window->showHome();
    });
    headerLayout->addWidget(m_backBtn);

    m_headerTitle = UiTheme::createTitle(tr("关于"), 24, header);
    headerLayout->addWidget(m_headerTitle);
    headerLayout->addStretch(1);
    rootLayout->addWidget(header);

    // 内容卡片
    auto *card = UiTheme::createCard(this);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(32, 32, 32, 32);
    layout->setSpacing(16);

    m_title = UiTheme::createTitle(QStringLiteral("Weqi"), 32, card);
    m_title->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_title);

    m_tagline = UiTheme::createMutedLabel(tr("现代国际象棋"), card);
    m_tagline->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_tagline);

    m_version = UiTheme::createMutedLabel(tr("版本 0.1.0"), card);
    m_version->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_version);

    m_desc = UiTheme::createMutedLabel(
        tr("Weqi 是一款开源的桌面国际象棋应用，"
           "所有棋规均在本地 C++ 引擎中实现，"
           "支持真人对战、人机对战、AI 对战与复盘。"),
        card);
    m_desc->setWordWrap(true);
    m_desc->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_desc);

    m_tech = UiTheme::createMutedLabel(
        tr("技术栈：C++17 · Qt 6 · CMake"), card);
    m_tech->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_tech);

    layout->addStretch(1);

    rootLayout->addWidget(card);
    rootLayout->addStretch(1);
}

void AboutPage::retranslateUi()
{
    m_backBtn->setText(tr("← 首页"));
    m_headerTitle->setText(tr("关于"));
    m_tagline->setText(tr("现代国际象棋"));
    m_version->setText(tr("版本 0.1.0"));
    m_desc->setText(tr("Weqi 是一款开源的桌面国际象棋应用，"
                       "所有棋规均在本地 C++ 引擎中实现，"
                       "支持真人对战、人机对战、AI 对战与复盘。"));
    m_tech->setText(tr("技术栈：C++17 · Qt 6 · CMake"));
}
