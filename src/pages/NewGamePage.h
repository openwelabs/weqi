#pragma once

#include "Page.h"

class QComboBox;
class QLineEdit;

// 新游戏设置页：选择对手（真人 / AI）、配置 AI 入口。
class NewGamePage : public Page
{
    Q_OBJECT

public:
    explicit NewGamePage(MainWindow *window, QWidget *parent = nullptr);

    void onShown() override;

private:
    void setupUi();
    void startGame();

    QComboBox *m_opponentCombo = nullptr;
    QLineEdit *m_whiteNameEdit = nullptr;
    QLineEdit *m_blackNameEdit = nullptr;
};
