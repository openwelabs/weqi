#pragma once

#include <QWidget>

class MainWindow;

// 页面基类：所有页面继承自此类。
// 提供统一的页面生命周期回调（onShown / onHidden），
// 以及访问主窗口（用于导航）的能力。
class Page : public QWidget
{
    Q_OBJECT

public:
    explicit Page(MainWindow *window, QWidget *parent = nullptr);

    // 页面显示时调用（用于刷新数据）
    virtual void onShown() {}

    // 页面隐藏时调用
    virtual void onHidden() {}

protected:
    MainWindow *m_window = nullptr;
};
