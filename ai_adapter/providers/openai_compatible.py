"""OpenAI-compatible API 提供商。

通过标准 OpenAI Chat Completions 接口调用第三方 AI。
只负责构造请求、调用 API、返回原始文本回复。
"""

import json
import urllib.request
import urllib.error


class OpenAICompatibleError(Exception):
    """调用 OpenAI-compatible API 时发生的错误。"""


def fen_to_ascii(fen: str) -> str:
    """把 FEN 转成带坐标的 ASCII 棋盘图，帮助语言模型理解局面。

    白方棋子用大写字母（P N B R Q K），黑方用小写（p n b r q k）。
    棋盘从第 8 行（黑方底线）到第 1 行（白方底线）排列。
    """
    board_part = fen.split(" ")[0]
    rows = board_part.split("/")
    lines = []
    lines.append("   a b c d e f g h")
    for rank, row in enumerate(rows):
        rank_label = 8 - rank
        cells = []
        for ch in row:
            if ch.isdigit():
                cells.extend(["."] * int(ch))
            else:
                cells.append(ch)
        lines.append(f"{rank_label}  " + " ".join(cells) + f"  {rank_label}")
    lines.append("   a b c d e f g h")
    return "\n".join(lines)


def _build_prompt(fen: str, turn: str, move_history: list, legal_moves: list,
                  last_error: str = "") -> str:
    """构造发送给 AI 的 Prompt。

    明确要求模型只返回一个标准 UCI 走法，不返回解释/Markdown/多个候选。
    附带 ASCII 棋盘图，帮助模型准确理解当前局面。
    关键：把当前方所有合法走法列出来，要求模型从中选择一个，杜绝非法走法。
    若上次选错（last_error 非空），明确告知 AI 上次选错了，要求重新选一个不同的。
    """
    history = " ".join(move_history) if move_history else "(无)"
    board = fen_to_ascii(fen)
    turn_cn = "白方" if turn == "white" else "黑方"
    legal = ", ".join(legal_moves) if legal_moves else "(无)"

    feedback = ""
    if last_error:
        feedback = (
            f"\n【上次你选错了】你上次输出了 {last_error}，但它不是合法走法。\n"
            "请务必从上面的合法走法列表中重新选择一个不同的走法。\n"
        )

    return (
        "你是一个国际象棋引擎，只负责输出下一步棋的走法，以及一句符合你性格的简短吐槽。\n"
        "下面是当前棋盘（大写=白方，小写=黑方，. =空格）：\n"
        f"{board}\n\n"
        f"当前轮到：{turn_cn}（{turn}）\n"
        f"已走的棋步：{history}\n\n"
        f"当前方（{turn_cn}）所有合法走法如下：\n"
        f"{legal}\n\n"
        "请从上面的合法走法列表中，选择你认为最好的一步棋。\n"
        "【重要】你只能从上面列出的合法走法中选择，绝不能输出列表之外的走法。\n"
        f"{feedback}"
        "同时，根据当前棋局、你刚走的这步棋、以及对手的行动，"
        "用一句简短的话吐槽/调侃/阴阳怪气一下（30 个中文字符以内）。\n"
        "语气示例（只是示例，不要照抄）：\n"
        "“哈哈哈哈！你马炸了！”\n"
        "“不是哥们，你这一步认真的？”\n"
        "“好好好，这么玩是吧。”\n"
        "“完了，你的王要遭罪了。”\n"
        "“漂亮，不过你真的确定要这么走？”\n"
        "“哈哈，终于让我逮到了。”\n"
        "要求：必须和当前棋局相关；不要每次用相同句式；不要长篇大论；"
        "不要输出思考过程；不要解释分析；不要为了凑字数强行说话。\n"
        "你的整个回复必须严格遵循以下 JSON 格式，不要输出任何其他内容：\n"
        '{"move": "e7e5", "message": "哈哈，你这一步有点东西。"}\n'
        "其中 move 是标准 UCI 走法（如 e2e4、g1f3、e1g1、e7e8q），"
        "必须从上面的合法走法列表中选择；message 是你的一句话。\n"
        "禁止输出 Markdown、代码块、解释、多个候选走法或任何其他字段。"
    )


def request_move(base_url: str, api_key: str, model: str,
                 fen: str, turn: str, move_history: list,
                 legal_moves: list = None,
                 last_error: str = "",
                 timeout: float = 60.0) -> str:
    """调用 OpenAI-compatible API，返回 AI 的原始文本回复。

    参数：
        base_url: 如 https://example.com/v1
        api_key:  API Key
        model:    模型名称
        fen:      当前局面 FEN
        turn:     当前回合（white / black）
        move_history: 已走的 UCI 走法列表
        legal_moves: 当前方所有合法走法（UCI 格式），AI 必须从中选择一个
        last_error: 上次 AI 选错的走法反馈（空表示首次请求），用于自动调教重试
        timeout:  请求超时（秒）

    返回：
        AI 的原始回复文本。

    抛出：
        OpenAICompatibleError: 网络错误、HTTP 错误、JSON 解析失败等。
    """
    if not base_url or not api_key or not model:
        raise OpenAICompatibleError("provider 配置不完整（base_url/api_key/model 缺失）")

    # 规范化 base_url：去掉末尾斜杠，确保以 /chat/completions 结尾
    base = base_url.rstrip("/")
    if base.endswith("/chat/completions"):
        url = base
    else:
        url = base + "/chat/completions"

    payload = {
        "model": model,
        "messages": [
            {"role": "system", "content": "你是一个国际象棋引擎。你的每次回复必须且只能是一个标准 UCI 走法字符串（如 e2e4、g1f3、e1g1、e7e8q），不得包含任何解释、标点、Markdown 或自然语言。"},
            {"role": "user", "content": _build_prompt(fen, turn, move_history, legal_moves or [], last_error)},
        ],
        "temperature": 0.2,
        "max_tokens": 32,
    }

    data = json.dumps(payload).encode("utf-8")
    req = urllib.request.Request(
        url,
        data=data,
        headers={
            "Content-Type": "application/json",
            "Authorization": "Bearer " + api_key,
        },
        method="POST",
    )

    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            body = resp.read().decode("utf-8", errors="replace")
    except urllib.error.HTTPError as e:
        detail = ""
        try:
            detail = e.read().decode("utf-8", errors="replace")[:200]
        except Exception:
            pass
        raise OpenAICompatibleError(f"HTTP {e.code}: {detail}") from e
    except urllib.error.URLError as e:
        raise OpenAICompatibleError(f"网络错误: {e.reason}") from e
    except TimeoutError as e:
        raise OpenAICompatibleError("请求超时") from e
    except Exception as e:
        raise OpenAICompatibleError(f"请求失败: {e}") from e

    # 解析 JSON 响应
    try:
        obj = json.loads(body)
        content = obj["choices"][0]["message"]["content"]
    except (KeyError, IndexError, json.JSONDecodeError) as e:
        raise OpenAICompatibleError("API 响应格式无效") from e

    return content
