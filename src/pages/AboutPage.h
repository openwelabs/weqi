#pragma once

#include "Page.h"

// 关于页：应用信息。
class AboutPage : public Page
{
    Q_OBJECT

public:
    explicit AboutPage(MainWindow *window, QWidget *parent = nullptr);

private:
    void setupUi();
};
