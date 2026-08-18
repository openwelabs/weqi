#pragma once

#include "Page.h"

class QComboBox;
class QLineEdit;
class QLabel;
class QPushButton;

// 新游戏设置页：选择对手（真人 / AI）、配置 AI 入口。
class NewGamePage : public Page
{
    Q_OBJECT

public:
    explicit NewGamePage(MainWindow *window, QWidget *parent = nullptr);

    void onShown() override;
    void retranslateUi() override;

private:
    void setupUi();
    void startGame();

    QPushButton *m_backBtn = nullptr;
    QLabel *m_headerTitle = nullptr;
    QLabel *m_opponentLabel = nullptr;
    QComboBox *m_opponentCombo = nullptr;
    QLabel *m_whiteLabel = nullptr;
    QLineEdit *m_whiteNameEdit = nullptr;
    QLabel *m_blackLabel = nullptr;
    QLineEdit *m_blackNameEdit = nullptr;
    QLabel *m_aiHint = nullptr;
    QPushButton *m_startBtn = nullptr;
};
