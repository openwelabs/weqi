#pragma once

#include "Page.h"

class QListWidget;
class QLabel;
class QPushButton;

// AI vs AI 设置页：选择两个 AI 进行对弈。
// 白方与黑方各选择一个 AI Provider，然后开始对局。
class AIVsAIPage : public Page
{
    Q_OBJECT

public:
    explicit AIVsAIPage(MainWindow *window, QWidget *parent = nullptr);

    void onShown() override;

private:
    void setupUi();
    void refreshProviderLists();
    void startGame();

    // 当前选中的 Provider ID（无则返回空）
    QString currentWhiteProviderId() const;
    QString currentBlackProviderId() const;

    QListWidget *m_whiteList = nullptr;
    QListWidget *m_blackList = nullptr;
    QLabel *m_noConfigLabel = nullptr;
    QPushButton *m_configureBtn = nullptr;
    QPushButton *m_startBtn = nullptr;
};
