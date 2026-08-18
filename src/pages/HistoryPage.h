#pragma once

#include "Page.h"

class QListWidget;
class QLabel;
class QPushButton;

// 历史对局页：展示所有历史对局记录。
class HistoryPage : public Page
{
    Q_OBJECT

public:
    explicit HistoryPage(MainWindow *window, QWidget *parent = nullptr);

    void onShown() override;
    void retranslateUi() override;

private:
    void setupUi();
    void refresh();

    QPushButton *m_backBtn = nullptr;
    QLabel *m_headerTitle = nullptr;
    QListWidget *m_list = nullptr;
    QLabel *m_emptyLabel = nullptr;
};
