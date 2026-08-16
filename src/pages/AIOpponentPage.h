#pragma once

#include "Page.h"

class QListWidget;
class QLabel;
class QComboBox;
class QPushButton;

// AI 对手选择页：选择 AI 对手与执子方。
// 支持从已配置的 AI Provider 中选择，并选择 AI 执白/执黑/随机。
// 若未配置任何 AI Provider，提示用户先配置。
class AIOpponentPage : public Page
{
    Q_OBJECT

public:
    explicit AIOpponentPage(MainWindow *window, QWidget *parent = nullptr);

    void onShown() override;

private:
    void setupUi();
    void refreshProviderList();
    void startGame();

    // 当前选中的 Provider ID（无则返回空）
    QString currentProviderId() const;

    QListWidget *m_aiList = nullptr;
    QComboBox *m_sideCombo = nullptr;
    QLabel *m_descLabel = nullptr;
    QLabel *m_noConfigLabel = nullptr;
    QPushButton *m_configureBtn = nullptr;
    QPushButton *m_startBtn = nullptr;
};
