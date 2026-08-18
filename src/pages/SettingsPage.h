#pragma once

#include "Page.h"

class QLineEdit;
class QListWidget;
class QLabel;
class QComboBox;
class QStackedWidget;
class QPushButton;

// 设置页：玩家资料 + 语言 + AI Providers 配置。
class SettingsPage : public Page
{
    Q_OBJECT

public:
    explicit SettingsPage(MainWindow *window, QWidget *parent = nullptr);

    void onShown() override;
    void retranslateUi() override;

private:
    void setupUi();
    void refreshProviders();
    void populateLanguageCombo();

    // 顶部标题栏
    QPushButton *m_backBtn = nullptr;
    QLabel *m_headerTitle = nullptr;

    // 玩家资料
    QLabel *m_profileTitle = nullptr;
    QLabel *m_nameLabel = nullptr;
    QLineEdit *m_nameEdit = nullptr;
    QLabel *m_ratingLabel = nullptr;
    QPushButton *m_saveProfileBtn = nullptr;

    // 语言
    QLabel *m_languageTitle = nullptr;
    QLabel *m_languageLabel = nullptr;
    QComboBox *m_languageCombo = nullptr;

    // AI Providers
    QLabel *m_aiTitle = nullptr;
    QLabel *m_aiHint = nullptr;
    QListWidget *m_providerList = nullptr;
    QLabel *m_providerCountLabel = nullptr;
    QLabel *m_formLabel = nullptr;
    QLineEdit *m_providerName = nullptr;
    QLineEdit *m_providerType = nullptr;
    QLineEdit *m_providerBaseUrl = nullptr;
    QLineEdit *m_providerApiKey = nullptr;
    QLineEdit *m_providerModel = nullptr;
    QPushButton *m_addBtn = nullptr;
    QPushButton *m_updateBtn = nullptr;
    QPushButton *m_deleteBtn = nullptr;
    int m_editingIndex = -1;
};
