#include "HistoryPage.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>

#include "MainWindow.h"
#include "UiTheme.h"
#include "GameHistoryManager.h"
#include "GameRecord.h"

namespace {

QString modeText(GameMode mode)
{
    switch (mode) {
    case GameMode::HumanVsHuman: return QStringLiteral("真人对战");
    case GameMode::HumanVsAI:    return QStringLiteral("人机对战");
    case GameMode::AIVsAI:       return QStringLiteral("AI 对战");
    case GameMode::Replay:       return QStringLiteral("复盘");
    }
    return QString();
}

QString outcomeText(GameOutcome outcome)
{
    switch (outcome) {
    case GameOutcome::Win:    return QStringLiteral("胜");
    case GameOutcome::Loss:   return QStringLiteral("负");
    case GameOutcome::Draw:   return QStringLiteral("和");
    case GameOutcome::Ongoing: return QStringLiteral("进行中");
    }
    return QString();
}

} // namespace

HistoryPage::HistoryPage(MainWindow *window, QWidget *parent)
    : Page(window, parent)
{
    setupUi();
}

void HistoryPage::setupUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(40, 32, 40, 40);
    rootLayout->setSpacing(20);

    // 顶部标题栏
    auto *header = new QWidget(this);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(14);

    auto *backBtn = UiTheme::createGhostButton(QStringLiteral("← 首页"), header);
    connect(backBtn, &QPushButton::clicked, this, [this]() {
        m_window->showHome();
    });
    headerLayout->addWidget(backBtn);

    auto *title = UiTheme::createTitle(QStringLiteral("历史对局"), 24, header);
    headerLayout->addWidget(title);
    headerLayout->addStretch(1);
    rootLayout->addWidget(header);

    // 列表卡片
    auto *card = UiTheme::createCard(this);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);

    m_emptyLabel = UiTheme::createMutedLabel(QStringLiteral("还没有对局记录。"), card);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_emptyLabel);

    m_list = new QListWidget(card);
    m_list->setStyleSheet(QStringLiteral(
        "QListWidget { background-color: %1; border: none; border-radius: 12px;"
        " color: %2; font-size: 14px; padding: 8px; }"
        "QListWidget::item { padding: 12px 10px; border-radius: 8px; }"
        "QListWidget::item:selected { background-color: %3; }")
        .arg(UiTheme::kPanelBg.name()).arg(UiTheme::kTitleColor.name()).arg(UiTheme::kAccentSoft.name()));
    layout->addWidget(m_list);

    rootLayout->addWidget(card);
    rootLayout->addStretch(1);
}

void HistoryPage::onShown()
{
    refresh();
}

void HistoryPage::refresh()
{
    m_list->clear();
    const QVector<GameRecord> &records = m_window->history()->records();

    const bool empty = records.isEmpty();
    m_emptyLabel->setVisible(empty);
    m_list->setVisible(!empty);

    for (const GameRecord &rec : records) {
        QString line = QStringLiteral("%1  %2  %3  %4  %5")
                           .arg(rec.date.toString(QStringLiteral("yyyy-MM-dd HH:mm")),
                                modeText(rec.mode),
                                rec.opponent,
                                outcomeText(rec.outcome),
                                rec.resultText);
        // AI 对局：附加 AI 模型信息
        if (rec.mode == GameMode::HumanVsAI) {
            const bool playerIsWhite = !rec.whiteAIProvider.isEmpty();
            const QString aiModel = playerIsWhite ? rec.whiteAIModel : rec.blackAIModel;
            if (!aiModel.isEmpty())
                line += QStringLiteral("  [%1]").arg(aiModel);
        } else if (rec.mode == GameMode::AIVsAI) {
            if (!rec.whiteAIModel.isEmpty() || !rec.blackAIModel.isEmpty())
                line += QStringLiteral("  [%1 vs %2]")
                            .arg(rec.whiteAIModel.isEmpty() ? QStringLiteral("?") : rec.whiteAIModel,
                                 rec.blackAIModel.isEmpty() ? QStringLiteral("?") : rec.blackAIModel);
        }
        if (rec.ratingChange != 0) {
            const QString sign = rec.ratingChange > 0 ? QStringLiteral("+") : QString();
            line += QStringLiteral("  (%1%2)").arg(sign).arg(rec.ratingChange);
        }
        auto *item = new QListWidgetItem(line, m_list);
        item->setData(Qt::UserRole, rec.id);
        m_list->addItem(item);
    }
}
