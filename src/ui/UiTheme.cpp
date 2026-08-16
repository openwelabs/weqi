#include "UiTheme.h"

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QFont>

namespace UiTheme {

const QColor kWindowBg(0x1B, 0x1C, 0x2A);
const QColor kPanelBg(0x24, 0x25, 0x36);
const QColor kCardBg(0x2A, 0x2C, 0x40);
const QColor kTitleColor(0xF5, 0xF5, 0xF5);
const QColor kMutedText(0x9A, 0x9D, 0xB3);
const QColor kAccent(0x4C, 0xAF, 0x50);
const QColor kAccentSoft(0x4C, 0xAF, 0x50, 40);
const QColor kDivider(0x3A, 0x3C, 0x52);
const QColor kDanger(0xE0, 0x6C, 0x5A);
const QColor kWin(0x4C, 0xAF, 0x50);
const QColor kDraw(0x9A, 0x9D, 0xB3);
const QColor kOverlayBg(0x1B, 0x1C, 0x2A, 200);

QString cardStyle()
{
    return QStringLiteral(
        "QFrame { background-color: %1; border-radius: 18px; }").arg(kCardBg.name());
}

QString primaryButtonStyle()
{
    return QStringLiteral(
        "QPushButton { background-color: %1; color: %2; border: none;"
        " border-radius: 12px; padding: 12px 20px; font-size: 15px; font-weight: bold; }"
        "QPushButton:hover { background-color: %3; }"
        "QPushButton:pressed { background-color: %4; }")
        .arg(kAccent.name()).arg(kTitleColor.name())
        .arg(QColor(0x5A, 0xC8, 0x5E).name())
        .arg(QColor(0x3A, 0x8F, 0x3E).name());
}

QString secondaryButtonStyle()
{
    return QStringLiteral(
        "QPushButton { background-color: %1; color: %2; border: none;"
        " border-radius: 12px; padding: 12px 20px; font-size: 15px; font-weight: bold; }"
        "QPushButton:hover { background-color: %3; }")
        .arg(kCardBg.name()).arg(kTitleColor.name()).arg(kAccentSoft.name());
}

QString ghostButtonStyle()
{
    return QStringLiteral(
        "QPushButton { background-color: transparent; color: %1; border: 1px solid %2;"
        " border-radius: 12px; padding: 10px 18px; font-size: 14px; }"
        "QPushButton:hover { background-color: %3; }")
        .arg(kMutedText.name()).arg(kDivider.name()).arg(kAccentSoft.name());
}

QString inputStyle()
{
    return QStringLiteral(
        "QLineEdit { background-color: %1; color: %2; border: 1px solid %3;"
        " border-radius: 10px; padding: 10px 12px; font-size: 14px; }"
        "QLineEdit:focus { border-color: %4; }")
        .arg(kPanelBg.name()).arg(kTitleColor.name()).arg(kDivider.name()).arg(kAccent.name());
}

QString comboStyle()
{
    return QStringLiteral(
        "QComboBox { background-color: %1; color: %2; border: 1px solid %3;"
        " border-radius: 10px; padding: 10px 12px; font-size: 14px; }"
        "QComboBox::drop-down { border: none; width: 24px; }"
        "QComboBox QAbstractItemView { background-color: %1; color: %2;"
        " border: 1px solid %3; selection-background-color: %4; }")
        .arg(kPanelBg.name()).arg(kTitleColor.name()).arg(kDivider.name()).arg(kAccent.name());
}

QWidget *createCard(QWidget *parent)
{
    auto *card = new QFrame(parent);
    card->setStyleSheet(cardStyle());
    return card;
}

QLabel *createTitle(const QString &text, int pointSize, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    QFont font = label->font();
    font.setPointSize(pointSize);
    font.setBold(true);
    label->setFont(font);
    label->setStyleSheet(QStringLiteral("color: %1;").arg(kTitleColor.name()));
    return label;
}

QLabel *createSectionLabel(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    QFont font = label->font();
    font.setPointSize(13);
    font.setBold(true);
    label->setFont(font);
    label->setStyleSheet(QStringLiteral("color: %1;").arg(kMutedText.name()));
    return label;
}

QLabel *createMutedLabel(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setStyleSheet(QStringLiteral("color: %1;").arg(kMutedText.name()));
    return label;
}

QPushButton *createPrimaryButton(const QString &text, QWidget *parent)
{
    auto *btn = new QPushButton(text, parent);
    btn->setStyleSheet(primaryButtonStyle());
    btn->setCursor(Qt::PointingHandCursor);
    return btn;
}

QPushButton *createSecondaryButton(const QString &text, QWidget *parent)
{
    auto *btn = new QPushButton(text, parent);
    btn->setStyleSheet(secondaryButtonStyle());
    btn->setCursor(Qt::PointingHandCursor);
    return btn;
}

QPushButton *createGhostButton(const QString &text, QWidget *parent)
{
    auto *btn = new QPushButton(text, parent);
    btn->setStyleSheet(ghostButtonStyle());
    btn->setCursor(Qt::PointingHandCursor);
    return btn;
}

} // namespace UiTheme
