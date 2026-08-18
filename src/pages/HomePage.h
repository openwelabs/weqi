#pragma once

#include "Page.h"

#include <QVector>
#include <functional>

class QLabel;
class QVBoxLayout;
class QWidget;
class QFrame;
class QEvent;
class QPushButton;

// 首页：玩家资料 + Quick Play + 游戏模式 + 战绩 + 最近对局 + Continue Game。
class HomePage : public Page
{
    Q_OBJECT

public:
    explicit HomePage(MainWindow *window, QWidget *parent = nullptr);

    void onShown() override;
    void retranslateUi() override;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setupUi();
    void refresh();

    // 各区块
    QWidget *createProfileCard();
    QWidget *createQuickPlayCard();
    QWidget *createGameModesCard();
    QWidget *createStatsCard();
    QWidget *createRecentGamesCard();
    QWidget *createContinueCard();
    QWidget *createFooter();

    // 动画
    void animateIn();

    // 游戏模式可点击卡片（QFrame + 点击动作）
    struct ModeCard {
        QFrame *frame;
        std::function<void()> action;
    };
    QVector<ModeCard> m_modeCards;

    // 静态文本控件（用于 retranslateUi）
    QLabel *m_subtitle = nullptr;
    QLabel *m_quickPlayTitle = nullptr;
    QPushButton *m_quickPlayBtn = nullptr;
    QVector<QLabel *> m_modeLabels;
    QVector<QLabel *> m_modeDescs;
    QLabel *m_continueTitle = nullptr;
    QLabel *m_continueDesc = nullptr;
    QPushButton *m_continueBtn = nullptr;
    QPushButton *m_settingsBtn = nullptr;
    QPushButton *m_historyBtn = nullptr;
    QPushButton *m_aboutBtn = nullptr;

    // 布局引用（用于刷新）
    QVBoxLayout *m_contentLayout = nullptr;
    QWidget *m_continueCard = nullptr;
    QWidget *m_profileCard = nullptr;
    QWidget *m_statsCard = nullptr;
    QWidget *m_recentCard = nullptr;
};
