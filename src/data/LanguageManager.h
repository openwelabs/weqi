#pragma once

#include <QObject>
#include <QString>

class QTranslator;
class SettingsManager;

// 语言管理：负责加载/切换界面语言。
//
// 语言设置持久化到 SettingsManager（config/settings.json 的 "language" 字段）。
// 首次启动默认 "system"，自动检测系统语言；手动选择后立即保存并切换。
//
// 支持语言（内部值 = 设置值 = .ts 后缀）：
//   zh-CN 简体中文 / zh-TW 繁體中文 / en English / ja 日本語
//   es Español / uk Українська / ko 한국어
// 特殊值 "system" 表示跟随系统。
class LanguageManager : public QObject
{
    Q_OBJECT

public:
    explicit LanguageManager(SettingsManager *settings, QObject *parent = nullptr);

    // 原始设置值："system" 或某个语言代码
    QString languageSetting() const;

    // 解析后的当前界面语言代码（zh-CN / zh-TW / en / ja / es / uk / ko）。
    // 若设置为 "system"，则根据系统语言解析；不支持的系统语言回退到 en。
    QString currentLanguage() const;

    // 设置语言（"system" 或语言代码）。立即保存并切换界面语言。
    void setLanguage(const QString &lang);

    // 安装/切换 QTranslator（应用当前语言）。由 MainWindow 在启动时调用。
    void apply();

    // 语言代码 -> 本地名称（用于设置界面下拉框显示）
    static QString languageDisplayName(const QString &code);

    // 所有支持的语言代码（不含 "system"）
    static QStringList supportedLanguages();

signals:
    // 界面语言已切换（携带解析后的语言代码）
    void languageChanged(const QString &code);

private:
    // 根据系统语言解析出界面语言代码
    QString resolveSystemLanguage() const;

    SettingsManager *m_settings = nullptr;
    QTranslator *m_translator = nullptr;
    QString m_current; // 当前解析后的语言代码
};
