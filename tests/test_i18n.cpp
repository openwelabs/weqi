// 国际化（i18n）测试。
//
// 覆盖可直接在 C++ 层验证的国际化行为：
//   - 支持语言列表与本地名称显示（7 种语言）
//   - 手动选择语言并立即生效
//   - 语言设置持久化（重启后保留）
//   - 跟随系统（"system"）解析为受支持语言
//   - 对局中途切换语言不丢失棋局状态
//   - AI 模型名称不被翻译（原样保留）
//
// 系统语言自动检测、AI 聊天语言跟随、无未翻译文本、布局溢出等
// 由 Python 测试与静态检查覆盖（见 ai_adapter/test_i18n_ai.py 与
// scripts/check_i18n.py）。

#include <QtTest/QtTest>
#include <QStringList>
#include <QCoreApplication>

#include "LanguageManager.h"
#include "SettingsManager.h"
#include "GameController.h"
#include "AIProviderManager.h"

class TestI18n : public QObject
{
    Q_OBJECT

private slots:
    // 测试 2：7 种语言显示（本地名称）
    void supportedLanguages_returnsSeven();
    void languageDisplayName_returnsLocalNames();

    // 测试 4：手动选择语言并立即生效
    void manualSelection_appliesImmediately();

    // 测试 5：语言设置持久化（重启后保留）
    void languageSetting_persistsAcrossRestart();

    // 测试 6：跟随系统解析为受支持语言
    void followSystem_resolvesToSupportedLanguage();

    // 测试 7：对局中途切换语言不丢失棋局状态
    void midGameSwitch_preservesGameState();

    // 测试 12：AI 模型名称不被翻译（原样保留）
    void modelName_notTranslated();
};

void TestI18n::supportedLanguages_returnsSeven()
{
    const QStringList langs = LanguageManager::supportedLanguages();
    QCOMPARE(langs.size(), 7);
    QVERIFY(langs.contains(QStringLiteral("zh-CN")));
    QVERIFY(langs.contains(QStringLiteral("zh-TW")));
    QVERIFY(langs.contains(QStringLiteral("en")));
    QVERIFY(langs.contains(QStringLiteral("ja")));
    QVERIFY(langs.contains(QStringLiteral("es")));
    QVERIFY(langs.contains(QStringLiteral("uk")));
    QVERIFY(langs.contains(QStringLiteral("ko")));
}

void TestI18n::languageDisplayName_returnsLocalNames()
{
    QCOMPARE(LanguageManager::languageDisplayName(QStringLiteral("zh-CN")),
             QString::fromUtf8("简体中文"));
    QCOMPARE(LanguageManager::languageDisplayName(QStringLiteral("zh-TW")),
             QString::fromUtf8("繁體中文"));
    QCOMPARE(LanguageManager::languageDisplayName(QStringLiteral("en")),
             QStringLiteral("English"));
    QCOMPARE(LanguageManager::languageDisplayName(QStringLiteral("ja")),
             QString::fromUtf8("日本語"));
    QCOMPARE(LanguageManager::languageDisplayName(QStringLiteral("es")),
             QString::fromUtf8("Español"));
    QCOMPARE(LanguageManager::languageDisplayName(QStringLiteral("uk")),
             QString::fromUtf8("Українська"));
    QCOMPARE(LanguageManager::languageDisplayName(QStringLiteral("ko")),
             QString::fromUtf8("한국어"));
}

void TestI18n::manualSelection_appliesImmediately()
{
    SettingsManager settings;
    LanguageManager lm(&settings);

    lm.setLanguage(QStringLiteral("ja"));
    QCOMPARE(lm.languageSetting(), QStringLiteral("ja"));
    QCOMPARE(lm.currentLanguage(), QStringLiteral("ja"));
    QCOMPARE(settings.language(), QStringLiteral("ja"));
}

void TestI18n::languageSetting_persistsAcrossRestart()
{
    // 第一次实例：设置语言并销毁（触发保存）
    {
        SettingsManager settings;
        settings.setLanguage(QStringLiteral("ko"));
        QCOMPARE(settings.language(), QStringLiteral("ko"));
    }

    // 第二次实例：模拟重启，从磁盘重新加载
    {
        SettingsManager settings;
        QCOMPARE(settings.language(), QStringLiteral("ko"));
    }
}

void TestI18n::followSystem_resolvesToSupportedLanguage()
{
    SettingsManager settings;
    LanguageManager lm(&settings);

    // 显式设为 "system"（跟随系统），确保不受其他测试残留设置影响
    settings.setLanguage(QStringLiteral("system"));
    QCOMPARE(settings.language(), QStringLiteral("system"));

    lm.apply();
    // 跟随系统解析出的语言必须是受支持语言之一
    QVERIFY(LanguageManager::supportedLanguages().contains(lm.currentLanguage()));
}

void TestI18n::midGameSwitch_preservesGameState()
{
    GameController gc;

    // 走几步棋（e4 e5 Nf3）
    QVERIFY(gc.tryMove(6, 4, 4, 4)); // e2-e4
    QVERIFY(gc.tryMove(1, 4, 3, 4)); // e7-e5
    QVERIFY(gc.tryMove(7, 6, 5, 5)); // g1-f3

    const QString fenBefore = gc.state().toFen();
    const int historyBefore = gc.history().count();
    QCOMPARE(gc.result(), GameController::Result::Ongoing);

    // 切换界面语言（不应影响棋局状态）
    gc.setUiLanguage(QStringLiteral("uk"));
    gc.setUiLanguage(QStringLiteral("zh-CN"));

    QCOMPARE(gc.state().toFen(), fenBefore);
    QCOMPARE(gc.history().count(), historyBefore);
    QCOMPARE(gc.result(), GameController::Result::Ongoing);
}

void TestI18n::modelName_notTranslated()
{
    // AI 模型名称（如 "DeepSeek V3"、"qwen3:8b"）应原样保留，不做翻译。
    // 这里验证 AIProvider 的 model 字段原样存取，且不经过任何翻译函数。
    AIProvider p;
    p.name = QStringLiteral("DeepSeek");
    p.model = QStringLiteral("DeepSeek V3");

    GameController gc;
    gc.setAIGame(true, false, p, AIProvider());

    QCOMPARE(gc.whiteAIProviderName(), QStringLiteral("DeepSeek"));
    QCOMPARE(gc.whiteAIModel(), QStringLiteral("DeepSeek V3"));
}

QTEST_MAIN(TestI18n)
#include "test_i18n.moc"
