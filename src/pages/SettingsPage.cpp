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
#include <QComboBox>
#include <QScrollArea>
#include <QMessageBox>
#include <QDateTime>
#include <QSignalBlocker>

#include "MainWindow.h"
#include "UiTheme.h"
#include "ProfileManager.h"
#include "AIProviderManager.h"
#include "LanguageManager.h"

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

    m_backBtn = UiTheme::createGhostButton(tr("← 首页"), header);
    connect(m_backBtn, &QPushButton::clicked, this, [this]() {
        m_window->showHome();
    });
    headerLayout->addWidget(m_backBtn);

    m_headerTitle = UiTheme::createTitle(tr("设置"), 24, header);
    headerLayout->addWidget(m_headerTitle);
    headerLayout->addStretch(1);
    contentLayout->addWidget(header);

    // ---- 玩家资料卡片 ----
    auto *profileCard = UiTheme::createCard(content);
    auto *profileLayout = new QVBoxLayout(profileCard);
    profileLayout->setContentsMargins(24, 24, 24, 24);
    profileLayout->setSpacing(14);

    m_profileTitle = UiTheme::createSectionLabel(tr("玩家资料"), profileCard);
    profileLayout->addWidget(m_profileTitle);

    m_nameLabel = UiTheme::createMutedLabel(tr("玩家名称"), profileCard);
    profileLayout->addWidget(m_nameLabel);
    m_nameEdit = new QLineEdit(profileCard);
    m_nameEdit->setStyleSheet(UiTheme::inputStyle());
    profileLayout->addWidget(m_nameEdit);

    m_ratingLabel = UiTheme::createMutedLabel(
        tr("当前 Rating：%1    最佳 Rating：%2")
            .arg(m_window->profile()->rating())
            .arg(m_window->profile()->bestRating()),
        profileCard);
    profileLayout->addWidget(m_ratingLabel);

    m_saveProfileBtn = UiTheme::createPrimaryButton(tr("保存资料"), profileCard);
    connect(m_saveProfileBtn, &QPushButton::clicked, this, [this]() {
        const QString name = m_nameEdit->text().trimmed();
        if (!name.isEmpty())
            m_window->profile()->setPlayerName(name);
        m_window->profile()->save();
    });
    profileLayout->addWidget(m_saveProfileBtn);

    contentLayout->addWidget(profileCard);

    // ---- 语言卡片 ----
    auto *languageCard = UiTheme::createCard(content);
    auto *languageLayout = new QVBoxLayout(languageCard);
    languageLayout->setContentsMargins(24, 24, 24, 24);
    languageLayout->setSpacing(14);

    m_languageTitle = UiTheme::createSectionLabel(tr("语言"), languageCard);
    languageLayout->addWidget(m_languageTitle);

    m_languageLabel = UiTheme::createMutedLabel(tr("界面语言"), languageCard);
    languageLayout->addWidget(m_languageLabel);

    m_languageCombo = new QComboBox(languageCard);
    m_languageCombo->setStyleSheet(UiTheme::comboStyle());
    populateLanguageCombo();
    connect(m_languageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        if (index < 0)
            return;
        const QString code = m_languageCombo->itemData(index).toString();
        if (!code.isEmpty())
            m_window->language()->setLanguage(code);
    });
    languageLayout->addWidget(m_languageCombo);

    contentLayout->addWidget(languageCard);

    // ---- AI Providers 卡片 ----
    auto *aiCard = UiTheme::createCard(content);
    auto *aiLayout = new QVBoxLayout(aiCard);
    aiLayout->setContentsMargins(24, 24, 24, 24);
    aiLayout->setSpacing(14);

    m_aiTitle = UiTheme::createSectionLabel(tr("AI Providers"), aiCard);
    aiLayout->addWidget(m_aiTitle);

    m_aiHint = UiTheme::createMutedLabel(
        tr("配置 AI 提供商。API Key 为私密数据，仅保存在系统用户数据目录，不会写入项目。"), aiCard);
    m_aiHint->setWordWrap(true);
    aiLayout->addWidget(m_aiHint);

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
    m_formLabel = UiTheme::createSectionLabel(tr("添加 / 编辑 Provider"), aiCard);
    aiLayout->addWidget(m_formLabel);

    auto addField = [&](const QString &labelText, QLineEdit **edit) {
        auto *label = UiTheme::createMutedLabel(labelText, aiCard);
        aiLayout->addWidget(label);
        *edit = new QLineEdit(aiCard);
        (*edit)->setStyleSheet(UiTheme::inputStyle());
        aiLayout->addWidget(*edit);
    };

    addField(tr("名称"), &m_providerName);
    addField(tr("提供商类型"), &m_providerType);
    addField(QStringLiteral("Base URL"), &m_providerBaseUrl);
    addField(QStringLiteral("API Key"), &m_providerApiKey);
    addField(tr("模型"), &m_providerModel);

    // 按钮行
    auto *btnRow = new QHBoxLayout();
    btnRow->setSpacing(12);

    m_addBtn = UiTheme::createPrimaryButton(tr("添加"), aiCard);
    connect(m_addBtn, &QPushButton::clicked, this, [this]() {
        AIProvider p;
        p.id = QString::number(QDateTime::currentMSecsSinceEpoch());
        p.name = m_providerName->text().trimmed();
        p.provider = m_providerType->text().trimmed();
        p.baseUrl = m_providerBaseUrl->text().trimmed();
        p.apiKey = m_providerApiKey->text().trimmed();
        p.model = m_providerModel->text().trimmed();
        if (p.name.isEmpty()) {
            QMessageBox::warning(this, tr("提示"), tr("请填写 Provider 名称。"));
            return;
        }
        m_window->aiProviders()->addProvider(p);
        m_window->aiProviders()->save();
        refreshProviders();
    });
    btnRow->addWidget(m_addBtn);

    m_updateBtn = UiTheme::createSecondaryButton(tr("更新所选"), aiCard);
    connect(m_updateBtn, &QPushButton::clicked, this, [this]() {
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
    btnRow->addWidget(m_updateBtn);

    m_deleteBtn = UiTheme::createGhostButton(tr("删除所选"), aiCard);
    connect(m_deleteBtn, &QPushButton::clicked, this, [this]() {
        if (m_editingIndex < 0 || m_editingIndex >= m_window->aiProviders()->count())
            return;
        m_window->aiProviders()->removeProvider(m_editingIndex);
        m_window->aiProviders()->save();
        m_editingIndex = -1;
        refreshProviders();
    });
    btnRow->addWidget(m_deleteBtn);

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
    // 同步语言下拉框当前选中项（跟随系统时显示解析后的语言）
    populateLanguageCombo();
}

void SettingsPage::retranslateUi()
{
    // 顶部标题栏
    if (m_backBtn)
        m_backBtn->setText(tr("← 首页"));
    if (m_headerTitle)
        m_headerTitle->setText(tr("设置"));

    // 玩家资料
    if (m_profileTitle)
        m_profileTitle->setText(tr("玩家资料"));
    if (m_nameLabel)
        m_nameLabel->setText(tr("玩家名称"));
    if (m_ratingLabel)
        m_ratingLabel->setText(
            tr("当前 Rating：%1    最佳 Rating：%2")
                .arg(m_window->profile()->rating())
                .arg(m_window->profile()->bestRating()));
    if (m_saveProfileBtn)
        m_saveProfileBtn->setText(tr("保存资料"));

    // 语言
    if (m_languageTitle)
        m_languageTitle->setText(tr("语言"));
    if (m_languageLabel)
        m_languageLabel->setText(tr("界面语言"));
    populateLanguageCombo();

    // AI Providers
    if (m_aiTitle)
        m_aiTitle->setText(tr("AI Providers"));
    if (m_aiHint)
        m_aiHint->setText(
            tr("配置 AI 提供商。API Key 为私密数据，仅保存在系统用户数据目录，不会写入项目。"));
    if (m_formLabel)
        m_formLabel->setText(tr("添加 / 编辑 Provider"));
    if (m_addBtn)
        m_addBtn->setText(tr("添加"));
    if (m_updateBtn)
        m_updateBtn->setText(tr("更新所选"));
    if (m_deleteBtn)
        m_deleteBtn->setText(tr("删除所选"));
    refreshProviders();
}

void SettingsPage::populateLanguageCombo()
{
    if (!m_languageCombo)
        return;

    // 记住当前选中值，避免重建时触发切换
    const QString currentSetting = m_window->language()->languageSetting();

    QSignalBlocker blocker(m_languageCombo);
    m_languageCombo->clear();

    // 第一项：跟随系统（显示当前解析语言的本地名称）
    const QString resolved = m_window->language()->currentLanguage();
    m_languageCombo->addItem(tr("跟随系统（%1）").arg(LanguageManager::languageDisplayName(resolved)),
                             QStringLiteral("system"));

    // 其余语言
    const QStringList langs = LanguageManager::supportedLanguages();
    for (const QString &code : langs)
        m_languageCombo->addItem(LanguageManager::languageDisplayName(code), code);

    // 选中当前设置值
    int idx = m_languageCombo->findData(currentSetting);
    if (idx < 0)
        idx = 0;
    m_languageCombo->setCurrentIndex(idx);
}

void SettingsPage::refreshProviders()
{
    m_providerList->clear();
    const QVector<AIProvider> &providers = m_window->aiProviders()->providers();
    m_providerCountLabel->setText(tr("已配置 %1 个 Provider").arg(providers.size()));

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
