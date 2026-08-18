#pragma once

#include "Page.h"

class QLabel;
class QPushButton;

// 关于页：应用信息。
class AboutPage : public Page
{
    Q_OBJECT

public:
    explicit AboutPage(MainWindow *window, QWidget *parent = nullptr);

    void retranslateUi() override;

private:
    void setupUi();

    QPushButton *m_backBtn = nullptr;
    QLabel *m_headerTitle = nullptr;
    QLabel *m_title = nullptr;
    QLabel *m_tagline = nullptr;
    QLabel *m_version = nullptr;
    QLabel *m_desc = nullptr;
    QLabel *m_tech = nullptr;
};
