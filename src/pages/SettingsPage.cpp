#include "SettingsPage.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QScrollArea>
#include <QMessageBox>
#include <QDateTime>

#include "MainWindow.h"
#include "UiTheme.h"
#include "ProfileManager.h"
#include "AIProviderManager.h"

SettingsPage::SettingsPage(MainWindow *window, QWidget *parent)
    : Page(window, parent)
{
    setupUi();
}

void SettingsPage::setupUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral(
        "QScrollArea { background-color: %1; border: none; }"
        "QScrollArea > QWidget > QWidget { background-color: %1; }")
        .arg(UiTheme::kWindowBg.name()));
    scroll->viewport()->setAutoFillBackground(true);
    scroll->viewport()->setStyleSheet(QStringLiteral(
        "background-color: %1;").arg(UiTheme::kWindowBg.name()));

    auto *content = new QWidget(scroll);
    content->setAutoFillBackground(true);
    content->setStyleSheet(QStringLiteral(
        "background-color: %1;").arg(UiTheme::kWindowBg.name()));
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(40, 32, 40, 40);
    contentLayout->setSpacing(20);

    // 顶部标题栏
    auto *header = new QWidget(content);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(14);

    auto *backBtn = UiTheme::createGhostButton(QStringLiteral("← 首页"), header);
    connect(backBtn, &QPushButton::clicked, this, [this]() {
        m_window->showHome();
    });
    headerLayout->addWidget(backBtn);

    auto *title = UiTheme::createTitle(QStringLiteral("设置"), 24, header);
    headerLayout->addWidget(title);
    headerLayout->addStretch(1);
    contentLayout->addWidget(header);

    // ---- 玩家资料卡片 ----
    auto *profileCard = UiTheme::createCard(content);
    auto *profileLayout = new QVBoxLayout(profileCard);
    profileLayout->setContentsMargins(24, 24, 24, 24);
    profileLayout->setSpacing(14);

    auto *profileTitle = UiTheme::createSectionLabel(QStringLiteral("玩家资料"), profileCard);
    profileLayout->addWidget(profileTitle);

    auto *nameLabel = UiTheme::createMutedLabel(QStringLiteral("玩家名称"), profileCard);
    profileLayout->addWidget(nameLabel);
    m_nameEdit = new QLineEdit(profileCard);
    m_nameEdit->setStyleSheet(UiTheme::inputStyle());
    profileLayout->addWidget(m_nameEdit);

    auto *ratingLabel = UiTheme::createMutedLabel(
        QStringLiteral("当前 Rating：%1    最佳 Rating：%2")
            .arg(m_window->profile()->rating())
            .arg(m_window->profile()->bestRating()),
        profileCard);
    profileLayout->addWidget(ratingLabel);

    auto *saveProfileBtn = UiTheme::createPrimaryButton(QStringLiteral("保存资料"), profileCard);
    connect(saveProfileBtn, &QPushButton::clicked, this, [this]() {
        const QString name = m_nameEdit->text().trimmed();
        if (!name.isEmpty())
            m_window->profile()->setPlayerName(name);
        m_window->profile()->save();
    });
    profileLayout->addWidget(saveProfileBtn);

    contentLayout->addWidget(profileCard);

    // ---- AI Providers 卡片 ----
    auto *aiCard = UiTheme::createCard(content);
    auto *aiLayout = new QVBoxLayout(aiCard);
    aiLayout->setContentsMargins(24, 24, 24, 24);
    aiLayout->setSpacing(14);

    auto *aiTitle = UiTheme::createSectionLabel(QStringLiteral("AI Providers"), aiCard);
    aiLayout->addWidget(aiTitle);

    auto *aiHint = UiTheme::createMutedLabel(
        QStringLiteral("配置 AI 提供商。API Key 为私密数据，仅保存在系统用户数据目录，不会写入项目。"), aiCard);
    aiHint->setWordWrap(true);
    aiLayout->addWidget(aiHint);

    m_providerCountLabel = UiTheme::createMutedLabel(QString(), aiCard);
    aiLayout->addWidget(m_providerCountLabel);

    m_providerList = new QListWidget(aiCard);
    m_providerList->setStyleSheet(QStringLiteral(
        "QListWidget { background-color: %1; border: none; border-radius: 12px;"
        " color: %2; font-size: 14px; padding: 8px; }"
        "QListWidget::item { padding: 10px 10px; border-radius: 8px; }"
        "QListWidget::item:selected { background-color: %3; }")
        .arg(UiTheme::kPanelBg.name()).arg(UiTheme::kTitleColor.name()).arg(UiTheme::kAccentSoft.name()));
    m_providerList->setFixedHeight(120);
    aiLayout->addWidget(m_providerList);

    // 表单
    auto *formLabel = UiTheme::createSectionLabel(QStringLiteral("添加 / 编辑 Provider"), aiCard);
    aiLayout->addWidget(formLabel);

    auto addField = [&](const QString &labelText, QLineEdit **edit) {
        auto *label = UiTheme::createMutedLabel(labelText, aiCard);
        aiLayout->addWidget(label);
        *edit = new QLineEdit(aiCard);
        (*edit)->setStyleSheet(UiTheme::inputStyle());
        aiLayout->addWidget(*edit);
    };

    addField(QStringLiteral("名称"), &m_providerName);
    addField(QStringLiteral("提供商类型"), &m_providerType);
    addField(QStringLiteral("Base URL"), &m_providerBaseUrl);
    addField(QStringLiteral("API Key"), &m_providerApiKey);
    addField(QStringLiteral("模型"), &m_providerModel);

    // 按钮行
    auto *btnRow = new QHBoxLayout();
    btnRow->setSpacing(12);

    auto *addBtn = UiTheme::createPrimaryButton(QStringLiteral("添加"), aiCard);
    connect(addBtn, &QPushButton::clicked, this, [this]() {
        AIProvider p;
        p.id = QString::number(QDateTime::currentMSecsSinceEpoch());
        p.name = m_providerName->text().trimmed();
        p.provider = m_providerType->text().trimmed();
        p.baseUrl = m_providerBaseUrl->text().trimmed();
        p.apiKey = m_providerApiKey->text().trimmed();
        p.model = m_providerModel->text().trimmed();
        if (p.name.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请填写 Provider 名称。"));
            return;
        }
        m_window->aiProviders()->addProvider(p);
        m_window->aiProviders()->save();
        refreshProviders();
    });
    btnRow->addWidget(addBtn);

    auto *updateBtn = UiTheme::createSecondaryButton(QStringLiteral("更新所选"), aiCard);
    connect(updateBtn, &QPushButton::clicked, this, [this]() {
        if (m_editingIndex < 0 || m_editingIndex >= m_window->aiProviders()->count())
            return;
        AIProvider p = *m_window->aiProviders()->provider(m_editingIndex);
        p.name = m_providerName->text().trimmed();
        p.provider = m_providerType->text().trimmed();
        p.baseUrl = m_providerBaseUrl->text().trimmed();
        p.apiKey = m_providerApiKey->text().trimmed();
        p.model = m_providerModel->text().trimmed();
        m_window->aiProviders()->updateProvider(m_editingIndex, p);
        m_window->aiProviders()->save();
        refreshProviders();
    });
    btnRow->addWidget(updateBtn);

    auto *deleteBtn = UiTheme::createGhostButton(QStringLiteral("删除所选"), aiCard);
    connect(deleteBtn, &QPushButton::clicked, this, [this]() {
        if (m_editingIndex < 0 || m_editingIndex >= m_window->aiProviders()->count())
            return;
        m_window->aiProviders()->removeProvider(m_editingIndex);
        m_window->aiProviders()->save();
        m_editingIndex = -1;
        refreshProviders();
    });
    btnRow->addWidget(deleteBtn);

    aiLayout->addLayout(btnRow);

    contentLayout->addWidget(aiCard);
    contentLayout->addStretch(1);

    scroll->setWidget(content);
    rootLayout->addWidget(scroll);
}

void SettingsPage::onShown()
{
    m_nameEdit->setText(m_window->profile()->playerName());
    refreshProviders();
}

void SettingsPage::refreshProviders()
{
    m_providerList->clear();
    const QVector<AIProvider> &providers = m_window->aiProviders()->providers();
    m_providerCountLabel->setText(QStringLiteral("已配置 %1 个 Provider").arg(providers.size()));

    for (int i = 0; i < providers.size(); ++i) {
        const AIProvider &p = providers[i];
        auto *item = new QListWidgetItem(
            QStringLiteral("%1  ·  %2  ·  %3").arg(p.name, p.provider, p.model), m_providerList);
        item->setData(Qt::UserRole, i);
        m_providerList->addItem(item);
    }

    // 选中时填充表单
    connect(m_providerList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row < 0 || row >= m_window->aiProviders()->count())
            return;
        const AIProvider &p = *m_window->aiProviders()->provider(row);
        m_editingIndex = row;
        m_providerName->setText(p.name);
        m_providerType->setText(p.provider);
        m_providerBaseUrl->setText(p.baseUrl);
        m_providerApiKey->setText(p.apiKey);
        m_providerModel->setText(p.model);
    });
}
