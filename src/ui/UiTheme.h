#pragma once

#include <QColor>
#include <QString>

class QWidget;
class QLabel;
class QPushButton;

// 全局 UI 主题与样式工具。
// 所有页面共享统一的现代深色配色与卡片样式。
namespace UiTheme {

// ---- 配色 ----
extern const QColor kWindowBg;      // 深色背景
extern const QColor kPanelBg;       // 侧栏/卡片背景
extern const QColor kCardBg;        // 信息卡片背景
extern const QColor kTitleColor;    // 主文字
extern const QColor kMutedText;     // 次要文字
extern const QColor kAccent;        // 强调绿
extern const QColor kAccentSoft;    // 强调绿（半透明）
extern const QColor kDivider;       // 分隔线
extern const QColor kDanger;        // 危险/失败红
extern const QColor kWin;           // 胜利绿
extern const QColor kDraw;          // 和棋灰
extern const QColor kOverlayBg;     // 覆盖层半透明背景

// ---- 通用样式 ----
QString cardStyle();                 // 圆角卡片背景
QString primaryButtonStyle();        // 主按钮（强调绿）
QString secondaryButtonStyle();      // 次按钮（卡片背景）
QString ghostButtonStyle();          // 幽灵按钮（透明）
QString inputStyle();                // 输入框
QString comboStyle();                // 下拉框

// ---- 控件工厂 ----
QWidget *createCard(QWidget *parent = nullptr);
QLabel *createTitle(const QString &text, int pointSize = 24, QWidget *parent = nullptr);
QLabel *createSectionLabel(const QString &text, QWidget *parent = nullptr);
QLabel *createMutedLabel(const QString &text, QWidget *parent = nullptr);
QPushButton *createPrimaryButton(const QString &text, QWidget *parent = nullptr);
QPushButton *createSecondaryButton(const QString &text, QWidget *parent = nullptr);
QPushButton *createGhostButton(const QString &text, QWidget *parent = nullptr);

} // namespace UiTheme
