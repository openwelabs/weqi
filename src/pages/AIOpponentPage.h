#pragma once

#include "Page.h"

class QListWidget;
class QLabel;

// AI 对手选择页：选择 AI 对手（Webu / Custom AI，未来扩展）。
// 仅 UI 与接口，不连接真实 AI。
class AIOpponentPage : public Page
{
    Q_OBJECT

public:
    explicit AIOpponentPage(MainWindow *window, QWidget *parent = nullptr);

    void onShown() override;

private:
    void setupUi();
    void startGame();

    QListWidget *m_aiList = nullptr;
    QLabel *m_descLabel = nullptr;
};
