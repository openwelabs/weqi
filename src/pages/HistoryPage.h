#pragma once

#include "Page.h"

class QListWidget;
class QLabel;

// 历史对局页：展示所有历史对局记录。
class HistoryPage : public Page
{
    Q_OBJECT

public:
    explicit HistoryPage(MainWindow *window, QWidget *parent = nullptr);

    void onShown() override;

private:
    void setupUi();
    void refresh();

    QListWidget *m_list = nullptr;
    QLabel *m_emptyLabel = nullptr;
};
