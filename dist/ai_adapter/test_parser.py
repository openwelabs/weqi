"""parser 单元测试。"""
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from parser import parse_ai_response

def check(name, raw, expected_move):
    result = parse_ai_response(raw)
    if expected_move is None:
        assert result["ok"] is False, f"{name}: 应失败，实际 {result}"
        print(f"PASS {name}: 正确拒绝")
    else:
        assert result["ok"] is True and result["move"] == expected_move, \
            f"{name}: 期望 {expected_move}，实际 {result}"
        print(f"PASS {name}: {result['move']}")

check("纯UCI", "e2e4", "e2e4")
check("代码块", "```\ne2e4\n```", "e2e4")
check("带语言代码块", "```python\ne2e4\n```", "e2e4")
check("带解释", "I would play **e2e4** because it controls the center.", "e2e4")
check("带解释2", "The best move is e7e5. Let me explain why...", "e7e5")
check("多个走法取第一个", "e2e4 g1f3", "e2e4")
check("升变", "e7e8q", "e7e8q")
check("升变大写", "E7E8Q", "e7e8q")
check("带连字符", "e2-e4", "e2e4")
check("带连字符升变", "e7-e8q", "e7e8q")
check("带连字符+解释", "I recommend e2-e4 here.", "e2e4")
check("无走法", "no valid move here", None)
check("空串", "", None)
check("非法走法", "z9z9", None)
check("自然语言", "I think white is winning", None)

# ---- JSON 格式（move + message）----
def check_json(name, raw, expected_move, expected_message):
    result = parse_ai_response(raw)
    assert result["ok"] is True, f"{name}: 应成功，实际 {result}"
    assert result["move"] == expected_move, f"{name}: move 期望 {expected_move}，实际 {result}"
    assert result.get("message", "") == expected_message, \
        f"{name}: message 期望 {expected_message!r}，实际 {result.get('message')!r}"
    print(f"PASS {name}: move={result['move']} message={result['message']!r}")

check_json("JSON完整", '{"move": "e7e5", "message": "哈哈，你这一步有点东西。"}', "e7e5", "哈哈，你这一步有点东西。")
check_json("JSON无message", '{"move": "e2e4"}', "e2e4", "")
check_json("JSON带代码块", '```json\n{"move": "g1f3", "message": "将军！"}\n```', "g1f3", "将军！")
check_json("JSON升变", '{"move": "e7e8q", "message": "升变！"}', "e7e8q", "升变！")
check_json("JSON带解释前缀", '我选择 e2e4。{"move": "e2e4", "message": "走这里"}', "e2e4", "走这里")

print("\n全部通过")
