"""国际化（i18n）AI 语言测试。

覆盖 AI 聊天内容跟随当前界面语言的行为：
  - 测试 10：AI 聊天使用当前界面语言（_build_prompt 按语言生成）
  - 测试 11：AI vs AI 双方使用同一界面语言
  - 测试 3：不支持的系统语言回退到 English

这些测试不发起真实网络请求，只验证 Prompt 构造逻辑。
"""
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from providers.openai_compatible import (_build_prompt, _LANG_PROMPTS,
                                          detect_language,
                                          _extract_message_from_raw)

FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
LEGAL = ["a2a3", "a2a4", "b2b3", "b2b4", "c2c3", "c2c4", "d2d3", "d2d4",
         "e2e3", "e2e4", "f2f3", "f2f4", "g2g3", "g2g4", "h2h3", "h2h4",
         "b1a3", "b1c3", "g1f3", "g1h3"]


def check(name, cond, detail=""):
    assert cond, f"{name}: 失败 {detail}"
    print(f"PASS {name}")


# ---- 测试 10：AI 聊天使用当前界面语言 ----
def test_ai_chat_uses_ui_language():
    # 每种受支持语言都应生成对应语言的 Prompt
    for lang in _LANG_PROMPTS:
        prompt = _build_prompt(FEN, "white", [], LEGAL, "", lang)
        # 系统指令应使用该语言
        check(f"chat_lang_{lang}_system",
              _LANG_PROMPTS[lang]["system"] in prompt,
              f"语言 {lang} 的 system 文案未出现在 prompt 中")
        # 回合名应使用该语言
        turn_name = _LANG_PROMPTS[lang]["turn"][0]
        check(f"chat_lang_{lang}_turn",
              turn_name in prompt,
              f"语言 {lang} 的回合名 {turn_name} 未出现在 prompt 中")
        # 吐槽指令应使用该语言
        check(f"chat_lang_{lang}_chat",
              _LANG_PROMPTS[lang]["chat"] in prompt,
              f"语言 {lang} 的 chat 文案未出现在 prompt 中")


# ---- 测试 11：AI vs AI 双方使用同一界面语言 ----
def test_both_ais_same_language():
    # 白方与黑方使用同一界面语言时，生成的 Prompt 语言一致
    for lang in _LANG_PROMPTS:
        white_prompt = _build_prompt(FEN, "white", [], LEGAL, "", lang)
        black_prompt = _build_prompt(FEN, "black", [], LEGAL, "", lang)
        # 双方都包含该语言的系统指令
        check(f"both_ai_{lang}_white",
              _LANG_PROMPTS[lang]["system"] in white_prompt)
        check(f"both_ai_{lang}_black",
              _LANG_PROMPTS[lang]["system"] in black_prompt)
        # 白方回合名 = 该语言的白，黑方回合名 = 该语言的黑
        check(f"both_ai_{lang}_turn_names",
              _LANG_PROMPTS[lang]["turn"][0] in white_prompt
              and _LANG_PROMPTS[lang]["turn"][1] in black_prompt)


# ---- 测试 3：不支持的系统语言回退到 English ----
def test_unsupported_language_falls_back_to_english():
    # 不支持的代码（如 fr、de、pt）应回退到 en 文案
    for bad in ["fr", "de", "pt", "ru", "xx-YY", ""]:
        prompt = _build_prompt(FEN, "white", [], LEGAL, "", bad)
        check(f"fallback_{bad or 'empty'}_system",
              _LANG_PROMPTS["en"]["system"] in prompt,
              f"语言 {bad!r} 未回退到英文 system 文案")
        check(f"fallback_{bad or 'empty'}_turn",
              _LANG_PROMPTS["en"]["turn"][0] in prompt,
              f"语言 {bad!r} 未回退到英文回合名")


# ---- 测试 12：语言检测 detect_language ----
def test_detect_language():
    # 韩语
    check("detect_ko", detect_language("하하, 네 그 수 좀 있네.") == "ko")
    # 简体中文
    check("detect_zh_cn", detect_language("哈哈，你这一步有点东西。") == "zh-CN")
    # 繁体中文
    check("detect_zh_tw", detect_language("哈哈，你這一步有點東西。") == "zh-TW")
    # 日语
    check("detect_ja", detect_language("はは、その手はなかなかだな。") == "ja")
    # 西班牙语
    check("detect_es", detect_language("¡Ja! Tu caballo está perdido.") == "es")
    # 乌克兰语
    check("detect_uk", detect_language("Ха! Твій кінь готовий!") == "uk")
    # 英语
    check("detect_en", detect_language("Ha, that move of yours is something.") == "en")
    # 空文本回退 en
    check("detect_empty", detect_language("") == "en")


# ---- 测试 13：从原始回复提取 message ----
def test_extract_message_from_raw():
    # JSON 格式
    check("extract_json",
          _extract_message_from_raw('{"move": "e7e5", "message": "하하, 네 그 수 좀 있네."}')
          == "하하, 네 그 수 좀 있네.")
    # 纯文本（剥离走法）
    check("extract_plain",
          _extract_message_from_raw("e7e5 하하, 네 그 수 좀 있네.") == "하하, 네 그 수 좀 있네")
    # 空文本
    check("extract_empty", _extract_message_from_raw("") == "")


if __name__ == "__main__":
    test_ai_chat_uses_ui_language()
    test_both_ais_same_language()
    test_unsupported_language_falls_back_to_english()
    test_detect_language()
    test_extract_message_from_raw()
    print("ALL PASS")
