#pragma once

#include <QMainWindow>

#include "GameController.h"
#include "GameRecord.h"

class QStackedWidget;
class QWidget;

class Page;
class HomePage;
class NewGamePage;
class AIOpponentPage;
class GamePage;
class HistoryPage;
class SettingsPage;
class AboutPage;

class ProfileManager;
class SettingsManager;
class AIProviderManager;
class GameHistoryManager;
class StatsManager;

// 主窗口：应用外壳。
// 持有所有数据管理器与页面，通过 QStackedWidget 管理页面导航。
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    // ---- 数据管理器访问 ----
    ProfileManager *profile() const { return m_profile; }
    SettingsManager *settings() const { return m_settings; }
    AIProviderManager *aiProviders() const { return m_aiProviders; }
    GameHistoryManager *history() const { return m_history; }
    StatsManager *stats() const { return m_stats; }
    GameController *controller() const { return m_controller; }

    // ---- 页面导航 ----
    void showHome();
    void showNewGame();
    void showAIOpponent();
    void showGame();
    void showHistory();
    void showSettings();
    void showAbout();

    // 启动一局新游戏（由 NewGamePage / AIOpponentPage 调用）
    void startGame(GameMode mode, const QString &opponent, const QString &whiteName,
                   const QString &blackName);

    // 继续未完成对局
    void continueGame();

private:
    void setupManagers();
    void setupPages();
    void navigateTo(Page *page);

    // 数据管理器
    ProfileManager *m_profile = nullptr;
    SettingsManager *m_settings = nullptr;
    AIProviderManager *m_aiProviders = nullptr;
    GameHistoryManager *m_history = nullptr;
    StatsManager *m_stats = nullptr;

    // 棋局控制器（GamePage 使用）
    GameController *m_controller = nullptr;

    // 页面导航
    QStackedWidget *m_stack = nullptr;
    HomePage *m_homePage = nullptr;
    NewGamePage *m_newGamePage = nullptr;
    AIOpponentPage *m_aiOpponentPage = nullptr;
    GamePage *m_gamePage = nullptr;
    HistoryPage *m_historyPage = nullptr;
    SettingsPage *m_settingsPage = nullptr;
    AboutPage *m_aboutPage = nullptr;
};
