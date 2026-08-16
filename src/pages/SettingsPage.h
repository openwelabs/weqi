#pragma once

#include "Page.h"

class QLineEdit;
class QListWidget;
class QLabel;
class QStackedWidget;

// 设置页：玩家资料 + AI Providers 配置。
class SettingsPage : public Page
{
    Q_OBJECT

public:
    explicit SettingsPage(MainWindow *window, QWidget *parent = nullptr);

    void onShown() override;

private:
    void setupUi();
    void refreshProviders();

    // 玩家资料
    QLineEdit *m_nameEdit = nullptr;

    // AI Providers
    QListWidget *m_providerList = nullptr;
    QLabel *m_providerCountLabel = nullptr;
    QLineEdit *m_providerName = nullptr;
    QLineEdit *m_providerType = nullptr;
    QLineEdit *m_providerBaseUrl = nullptr;
    QLineEdit *m_providerApiKey = nullptr;
    QLineEdit *m_providerModel = nullptr;
    int m_editingIndex = -1;
};
