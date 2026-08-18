"""AI 回复解析器。

职责：从 AI 的原始回复中提取一个标准 UCI 走法（如 e2e4、e7e8q），
以及一句可选的聊天内容（message）。

支持两种回复格式：
1. JSON：{"move": "e7e5", "message": "哈哈，你这一步有点东西。"}
2. 纯文本：只包含一个 UCI 走法（如 e2e4），此时 message 为空。

只做文本清理与提取，不判断走法是否合法（合法性由 C++ 负责）。
"""

import json
import re

# UCI 走法：4 个字符（如 e2e4），升变时第 5 个字符为棋子字母（q/r/b/n）
_UCI_RE = re.compile(r"^[a-h][1-8][a-h][1-8][qrbnQRBN]?$")

# 带连字符的走法：e2-e4、e7-e8q
_HYPHEN_RE = re.compile(r"^[a-h][1-8]-[a-h][1-8][qrbnQRBN]?$")

# 短代数记法（仅目标格）：e4、e8q、Nf3、exd5、O-O、O-O-O
# 这类记法缺少来源格，无法直接转成 UCI，但可尝试从上下文推断。
# 这里只识别"纯目标格"形式（如 e4、e8q），供 C++ 侧做启发式补全。
_SHORT_RE = re.compile(r"^[a-h][1-8][qrbnQRBN]?$")


def clean_text(text: str) -> str:
    """清理 AI 回复中的 Markdown、代码块、多余文本。"""
    if not text:
        return ""
    # 去掉代码块围栏
    text = re.sub(r"```[a-zA-Z]*\s*", "", text)
    text = text.replace("```", "")
    # 去掉行内代码反引号
    text = text.replace("`", "")
    # 去掉 Markdown 加粗/斜体标记
    text = re.sub(r"\*\*|\*|__|_", "", text)
    return text.strip()


def _normalize_hyphen(token: str) -> str:
    """把 e2-e4 转成 e2e4。"""
    return token.replace("-", "")


def extract_uci(text: str) -> str:
    """从清理后的文本中提取第一个合法的 UCI 走法。

    优先匹配严格 UCI（e2e4、e7e8q），其次匹配带连字符形式（e2-e4）。
    返回匹配的 UCI 字符串；找不到返回空字符串。
    """
    if not text:
        return ""
    # 统一转小写，兼容 AI 返回大写走法（如 E7E8Q）
    lowered = text.lower()
    # 1) 优先查找严格 UCI 片段
    for token in re.findall(r"[a-h][1-8][a-h][1-8][qrbn]?", lowered):
        if _UCI_RE.match(token):
            return token
    # 2) 其次查找带连字符形式（e2-e4）
    for token in re.findall(r"[a-h][1-8]-[a-h][1-8][qrbn]?", lowered):
        if _HYPHEN_RE.match(token):
            return _normalize_hyphen(token)
    return ""


def _extract_message(text: str) -> str:
    """从清理后的文本中提取一句聊天内容。

    去掉走法片段与多余标点，返回一句简短的中文/自然语言内容。
    找不到则返回空字符串。
    """
    if not text:
        return ""
    # 去掉 UCI 走法片段（e2e4、e7e8q、e2-e4 等）
    cleaned = re.sub(r"[a-h][1-8]-?[a-h][1-8][qrbn]?", "", text)
    # 去掉 JSON 花括号与引号残留
    cleaned = cleaned.replace("{", "").replace("}", "").replace('"', "")
    cleaned = cleaned.replace("move", "").replace("message", "").replace(":", "")
    cleaned = cleaned.strip(" ,。.!！?？;；")
    return cleaned


def parse_ai_response(raw: str) -> dict:
    """解析 AI 原始回复，返回标准响应。

    成功：{"ok": true, "move": "e2e4", "message": "..."}
    失败：{"ok": false, "error": "invalid_ai_response"}

    优先尝试解析 JSON（{"move": "...", "message": "..."}）；
    若 AI 只返回纯 UCI 走法，则 message 为空字符串。
    """
    if not raw:
        return {"ok": False, "error": "invalid_ai_response"}

    cleaned = clean_text(raw)

    # 1) 尝试解析 JSON 格式（可能被解释文本包裹，先找 {...} 片段）
    move = ""
    message = ""
    json_parsed = False
    json_candidates = []
    # 直接整体解析
    try:
        obj = json.loads(cleaned)
        if isinstance(obj, dict):
            json_candidates.append(obj)
    except (ValueError, TypeError):
        pass
    # 从文本中提取 {...} 片段再解析
    for m in re.finditer(r"\{[^{}]*\}", cleaned):
        try:
            obj = json.loads(m.group(0))
            if isinstance(obj, dict):
                json_candidates.append(obj)
        except (ValueError, TypeError):
            continue
    for obj in json_candidates:
        cand_move = str(obj.get("move", "")).strip()
        if cand_move:
            json_parsed = True
            move = cand_move
            message = str(obj.get("message", "")).strip()
            break

    # 2) 若 JSON 解析失败或缺少 move，回退到纯文本提取
    if not move:
        move = extract_uci(cleaned)
        if not json_parsed:
            message = _extract_message(cleaned)

    if move:
        return {"ok": True, "move": move, "message": message}
    return {"ok": False, "error": "invalid_ai_response"}
