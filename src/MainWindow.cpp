#include "MainWindow.h"

#include <QStackedWidget>
#include <QPalette>

#include "UiTheme.h"
#include "Page.h"
#include "HomePage.h"
#include "NewGamePage.h"
#include "AIOpponentPage.h"
#include "GamePage.h"
#include "HistoryPage.h"
#include "SettingsPage.h"
#include "AboutPage.h"
#include "ProfileManager.h"
#include "SettingsManager.h"
#include "AIProviderManager.h"
#include "GameHistoryManager.h"
#include "StatsManager.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupManagers();
    setupPages();
}

void MainWindow::setupManagers()
{
    // 数据管理器
    m_profile = new ProfileManager(this);
    m_settings = new SettingsManager(this);
    m_aiProviders = new AIProviderManager(this);
    m_history = new GameHistoryManager(this);
    m_stats = new StatsManager(this);

    // 加载持久化数据
    m_profile->load();
    m_settings->load();
    m_aiProviders->load();
    m_history->load();

    // 战绩从历史动态计算
    m_stats->setHistory(m_history);
    m_stats->refresh();

    // 棋局控制器
    m_controller = new GameController(this);
}

void MainWindow::setupPages()
{
    setWindowTitle(QStringLiteral("Weqi"));
    resize(1080, 760);
    setMinimumSize(760, 600);

    // 深色现代背景
    QPalette pal = palette();
    pal.setColor(QPalette::Window, UiTheme::kWindowBg);
    pal.setColor(QPalette::Base, UiTheme::kPanelBg);
    setPalette(pal);

    // 页面导航容器
    m_stack = new QStackedWidget(this);
    m_stack->setAutoFillBackground(true);

    // 创建所有页面
    m_homePage = new HomePage(this, m_stack);
    m_newGamePage = new NewGamePage(this, m_stack);
    m_aiOpponentPage = new AIOpponentPage(this, m_stack);
    m_gamePage = new GamePage(this, m_stack);
    m_historyPage = new HistoryPage(this, m_stack);
    m_settingsPage = new SettingsPage(this, m_stack);
    m_aboutPage = new AboutPage(this, m_stack);

    // 让所有页面自动填充深色背景
    const QList<Page *> pages = {
        m_homePage, m_newGamePage, m_aiOpponentPage,
        m_gamePage, m_historyPage, m_settingsPage, m_aboutPage
    };
    for (Page *p : pages)
        p->setAutoFillBackground(true);

    m_stack->addWidget(m_homePage);
    m_stack->addWidget(m_newGamePage);
    m_stack->addWidget(m_aiOpponentPage);
    m_stack->addWidget(m_gamePage);
    m_stack->addWidget(m_historyPage);
    m_stack->addWidget(m_settingsPage);
    m_stack->addWidget(m_aboutPage);

    setCentralWidget(m_stack);

    // 默认显示首页
    navigateTo(m_homePage);
}

void MainWindow::navigateTo(Page *page)
{
    if (!page)
        return;

    // 通知旧页面隐藏
    if (Page *current = qobject_cast<Page *>(m_stack->currentWidget()))
        current->onHidden();

    m_stack->setCurrentWidget(page);
    page->onShown();
}

// ---- 页面导航 ----

void MainWindow::showHome()
{
    navigateTo(m_homePage);
}

void MainWindow::showNewGame()
{
    navigateTo(m_newGamePage);
}

void MainWindow::showAIOpponent()
{
    navigateTo(m_aiOpponentPage);
}

void MainWindow::showGame()
{
    navigateTo(m_gamePage);
}

void MainWindow::showHistory()
{
    navigateTo(m_historyPage);
}

void MainWindow::showSettings()
{
    navigateTo(m_settingsPage);
}

void MainWindow::showAbout()
{
    navigateTo(m_aboutPage);
}

// ---- 对局启动 ----

void MainWindow::startGame(GameMode mode, const QString &opponent,
                           const QString &whiteName, const QString &blackName)
{
    m_gamePage->startNewGame(mode, opponent, whiteName, blackName);
    navigateTo(m_gamePage);
}

void MainWindow::continueGame()
{
    m_gamePage->continueSavedGame();
    navigateTo(m_gamePage);
}
