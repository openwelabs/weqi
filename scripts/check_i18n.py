"""国际化静态检查。

覆盖无法通过单元测试直接验证的国际化要求：
  - 测试 1：首次启动自动检测系统语言（LanguageManager 默认 "system"，
            resolveSystemLanguage 对各类系统语言映射正确）
  - 测试 13：无未翻译文本（每个 tr() 源字符串在全部 7 个 .ts 中都有翻译）
  - 测试 14：无布局溢出（无固定宽度控件会截断长文本）

用法：python3 scripts/check_i18n.py
"""
import os
import re
import sys
import xml.etree.ElementTree as ET

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")
TRANSLATIONS = os.path.join(ROOT, "translations")

LANGS = ["zh_CN", "zh_TW", "en", "ja", "es", "uk", "ko"]
TS_FILES = {lang: os.path.join(TRANSLATIONS, f"weqi_{lang}.ts") for lang in LANGS}

failures = []


def check(name, cond, detail=""):
    if cond:
        print(f"PASS {name}")
    else:
        failures.append(f"{name}: {detail}")
        print(f"FAIL {name}: {detail}")


# ---- 测试 1：首次启动自动检测系统语言 ----
def test_first_launch_auto_detect():
    # LanguageManager 默认设置应为 "system"
    sm_path = os.path.join(SRC, "data", "SettingsManager.cpp")
    with open(sm_path, encoding="utf-8") as f:
        sm_src = f.read()
    check("first_launch_default_system",
          'm_language = obj["language"].toString(QStringLiteral("system"));' in sm_src,
          "SettingsManager 默认语言应为 system")

    # resolveSystemLanguage 的映射逻辑（从 LanguageManager.cpp 提取）
    lm_path = os.path.join(SRC, "data", "LanguageManager.cpp")
    with open(lm_path, encoding="utf-8") as f:
        lm_src = f.read()

    # 繁体中文分支
    check("detect_zh_tw",
          'lower.startsWith("zh-tw")' in lm_src
          and 'return QStringLiteral("zh-TW");' in lm_src,
          "缺少 zh-TW 检测")
    # 简体中文分支
    check("detect_zh_cn",
          'lower.startsWith("zh")' in lm_src
          and 'return QStringLiteral("zh-CN");' in lm_src,
          "缺少 zh-CN 检测")
    # 各语言分支
    for code in ["en", "ja", "es", "uk", "ko"]:
        check(f"detect_{code}",
              f'lower.startsWith("{code}")' in lm_src
              and f'return QStringLiteral("{code}");' in lm_src,
              f"缺少 {code} 检测")
    # 不支持语言回退 en
    check("detect_fallback_en",
          'return QStringLiteral("en");' in lm_src,
          "缺少不支持语言回退 en")


# ---- 测试 13：无未翻译文本 ----
def load_ts_translations(lang):
    """返回 {source: translation} 映射。空翻译（未翻译）记为 None。"""
    result = {}
    tree = ET.parse(TS_FILES[lang])
    for msg in tree.getroot().iter("message"):
        source_el = msg.find("source")
        if source_el is None:
            continue
        source = source_el.text or ""
        translation_el = msg.find("translation")
        if translation_el is None:
            result[source] = None
            continue
        text = translation_el.text or ""
        # 空翻译（type="unfinished" 且无文本）视为未翻译
        result[source] = text if text.strip() else None
    return result


def _unescape(s):
    """把 C 字符串字面量中的转义序列还原（\n、\"、\\ 等）。"""
    return s.replace('\\"', '"').replace('\\\\', '\\').replace('\\n', '\n')


def extract_tr_strings():
    """从所有 .cpp 源文件中提取 tr("...") 与 QCoreApplication::translate 的源字符串。

    处理多行字符串字面量拼接：C++ 中相邻的字符串字面量会被连接成一个字符串，
    因此 tr("a" "b") 的源字符串是 "ab"。这里把 tr(...) 括号内所有相邻的
    字符串字面量拼接起来。
    """
    strings = set()
    # 匹配 tr( 或 QCoreApplication::translate("ctx", 后跟一个或多个相邻字符串字面量
    tr_re = re.compile(
        r'(?:tr\(\s*|QCoreApplication::translate\(\s*"[^"]*"\s*,\s*)'
        r'((?:"(?:[^"\\]|\\.)*"\s*)+)')
    for root, _dirs, files in os.walk(SRC):
        for fn in files:
            if not fn.endswith(".cpp"):
                continue
            path = os.path.join(root, fn)
            with open(path, encoding="utf-8") as f:
                content = f.read()
            for m in tr_re.finditer(content):
                # 提取所有相邻字符串字面量并拼接
                literals = re.findall(r'"((?:[^"\\]|\\.)*)"', m.group(1))
                combined = "".join(_unescape(l) for l in literals)
                strings.add(combined)
    return strings


def test_no_untranslated_text():
    all_translations = {lang: load_ts_translations(lang) for lang in LANGS}
    sources = extract_tr_strings()

    check("tr_strings_extracted", len(sources) > 0,
          f"未提取到任何 tr() 字符串（共 {len(sources)}）")

    for lang in LANGS:
        missing = [s for s in sources if s not in all_translations[lang]]
        check(f"no_untranslated_{lang}",
              not missing,
              f"语言 {lang} 缺少 {len(missing)} 个翻译: {missing[:5]}")
        # 空翻译（source 存在但 translation 为空）也视为未翻译
        empty = [s for s in sources
                 if s in all_translations[lang] and all_translations[lang][s] is None]
        check(f"no_empty_translation_{lang}",
              not empty,
              f"语言 {lang} 有 {len(empty)} 个空翻译: {empty[:5]}")


# ---- 测试 14：无布局溢出 ----
def test_no_layout_overflow():
    # 检查是否存在固定宽度控件可能截断长文本。
    # 已知 GamePage 侧栏已加宽到 320px 容纳乌克兰语按钮。
    # 检查是否有 setFixedWidth 且宽度过小（< 300）的控件。
    overflow_risks = []
    fixed_re = re.compile(r'setFixedWidth\(\s*(\d+)\s*\)')
    for root, _dirs, files in os.walk(SRC):
        for fn in files:
            if not fn.endswith(".cpp"):
                continue
            path = os.path.join(root, fn)
            with open(path, encoding="utf-8") as f:
                content = f.read()
            for m in fixed_re.finditer(content):
                w = int(m.group(1))
                if w < 300:
                    overflow_risks.append(f"{os.path.relpath(path, ROOT)}: setFixedWidth({w})")

    # 允许的例外：棋盘本身、小图标等固定小宽度控件
    allowed = []
    real_risks = [r for r in overflow_risks if r not in allowed]
    check("no_layout_overflow",
          not real_risks,
          f"发现可能截断文本的固定宽度控件: {real_risks}")


if __name__ == "__main__":
    test_first_launch_auto_detect()
    test_no_untranslated_text()
    test_no_layout_overflow()
    if failures:
        print(f"\n{len(failures)} 项检查失败")
        sys.exit(1)
    print("\nALL PASS")
