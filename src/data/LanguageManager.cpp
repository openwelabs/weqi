#include "LanguageManager.h"

#include <QTranslator>
#include <QLocale>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

#include "SettingsManager.h"

namespace {

// 语言代码 -> 本地名称（用于设置界面下拉框）
struct LangEntry {
    const char *code;
    const char *display;
};

const LangEntry kLanguages[] = {
    { "zh-CN", "简体中文" },
    { "zh-TW", "繁體中文" },
    { "en",    "English" },
    { "ja",    "日本語" },
    { "es",    "Español" },
    { "uk",    "Українська" },
    { "ko",    "한국어" },
};

} // namespace

LanguageManager::LanguageManager(SettingsManager *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
{
    m_translator = new QTranslator(this);
}

QString LanguageManager::languageSetting() const
{
    return m_settings->language();
}

QString LanguageManager::currentLanguage() const
{
    return m_current;
}

QStringList LanguageManager::supportedLanguages()
{
    QStringList list;
    for (const LangEntry &e : kLanguages)
        list << QString::fromLatin1(e.code);
    return list;
}

QString LanguageManager::languageDisplayName(const QString &code)
{
    for (const LangEntry &e : kLanguages) {
        if (QString::fromLatin1(e.code) == code)
            return QString::fromUtf8(e.display);
    }
    return code;
}

QString LanguageManager::resolveSystemLanguage() const
{
    // 优先使用 uiLanguages（更精确，含地区），其次 language()
    const QStringList uiLangs = QLocale::system().uiLanguages();
    QString primary;
    if (!uiLangs.isEmpty())
        primary = uiLangs.first();
    if (primary.isEmpty())
        primary = QLocale::system().name();

    const QString lower = primary.toLower();

    // 繁体中文：zh-TW / zh-HK / zh-MO / zh-Hant
    if (lower.startsWith("zh-tw") || lower.startsWith("zh-hk")
        || lower.startsWith("zh-mo") || lower.contains("hant"))
        return QStringLiteral("zh-TW");

    // 简体中文：其他 zh-*
    if (lower.startsWith("zh"))
        return QStringLiteral("zh-CN");

    if (lower.startsWith("en"))
        return QStringLiteral("en");
    if (lower.startsWith("ja"))
        return QStringLiteral("ja");
    if (lower.startsWith("es"))
        return QStringLiteral("es");
    if (lower.startsWith("uk"))
        return QStringLiteral("uk");
    if (lower.startsWith("ko"))
        return QStringLiteral("ko");

    // 不支持的系统语言 -> English
    return QStringLiteral("en");
}

void LanguageManager::apply()
{
    qDebug() << "[LanguageManager] apply() ENTER, setting=" << m_settings->language();
    const QString setting = m_settings->language();
    const QString code = (setting == QStringLiteral("system") || setting.isEmpty())
                             ? resolveSystemLanguage()
                             : setting;

    // 校验 code 是否为支持的语言；否则回退 en
    QString resolved = code;
    if (!supportedLanguages().contains(resolved))
        resolved = QStringLiteral("en");

    if (resolved == m_current && m_translator->isEmpty() == false)
        return;

    m_current = resolved;

    // 卸载旧翻译
    if (!m_translator->isEmpty())
        QCoreApplication::removeTranslator(m_translator);

    // 加载 .qm 翻译文件（从可执行文件目录或资源）
    const QString qmName = QStringLiteral("weqi_%1.qm").arg(resolved);
    bool loaded = false;

    // 1) 尝试从可执行文件目录加载（开发/部署时 .qm 与可执行文件同目录）
    const QString exeDir = QCoreApplication::applicationDirPath();
    const QString exePath = exeDir + QDir::separator() + qmName;
    if (QFileInfo::exists(exePath))
        loaded = m_translator->load(exePath);

    // 2) 尝试从资源加载（CMake 嵌入）
    if (!loaded)
        loaded = m_translator->load(QStringLiteral(":/i18n/") + qmName);

    if (loaded)
        QCoreApplication::installTranslator(m_translator);

    qDebug() << "[LanguageManager] apply() resolved=" << resolved
             << "loaded=" << loaded
             << "current=" << m_current
             << "translatorEmpty=" << m_translator->isEmpty();
    emit languageChanged(m_current);
}

void LanguageManager::setLanguage(const QString &lang)
{
    if (lang == m_settings->language())
        return;

    m_settings->setLanguage(lang);
    apply();
}
