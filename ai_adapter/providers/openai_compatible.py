"""OpenAI-compatible API 提供商。

通过标准 OpenAI Chat Completions 接口调用第三方 AI。
只负责构造请求、调用 API、返回原始文本回复。
"""

import json
import re
import urllib.request
import urllib.error


class OpenAICompatibleError(Exception):
    """调用 OpenAI-compatible API 时发生的错误。"""


# ---- 语言检测 ----
# 依据 Unicode 字符范围判断一段文本的主要语言。
# 用于校验 AI 生成的 message 是否与当前界面语言一致，不一致则重试调教。

# 各语言的特征字符范围（Unicode 码点区间）
_LANG_RANGES = {
    "ko": [(0xAC00, 0xD7AF), (0x1100, 0x11FF), (0x3130, 0x318F)],  # 谚文
    "zh-CN": [(0x4E00, 0x9FFF)],  # CJK 统一表意文字（简体/繁体共用）
    "zh-TW": [(0x4E00, 0x9FFF)],
    "ja": [(0x3040, 0x309F), (0x30A0, 0x30FF)],  # 平假名/片假名
    "uk": [(0x0400, 0x04FF)],  # 西里尔字母
    "es": [(0x00C0, 0x00FF)],  # 拉丁扩展（重音字符）
    "en": [(0x0041, 0x005A), (0x0061, 0x007A)],  # 基本拉丁字母
}

# 繁体中文特有字符（用于区分 zh-CN / zh-TW）
_TRADITIONAL_CHARS = set("這為與從對說時後會來們個過還進種樣點們國學現發當")


def _count_in_ranges(text: str, ranges) -> int:
    """统计文本中落在给定 Unicode 区间的字符数。"""
    count = 0
    for ch in text:
        cp = ord(ch)
        for lo, hi in ranges:
            if lo <= cp <= hi:
                count += 1
                break
    return count


def detect_language(text: str) -> str:
    """检测一段文本的主要语言，返回语言代码（zh-CN/zh-TW/en/ja/es/uk/ko）。

    采用"决定性脚本优先"策略：
      1. 谚文/假名/CJK/西里尔等独特脚本一旦出现即判定为该语言；
      2. 拉丁系文本：含重音字符判为 es，否则判为 en。
    空文本或无法判断时返回 "en"。
    """
    if not text:
        return "en"

    # 1) 决定性脚本：出现即判定
    if _count_in_ranges(text, _LANG_RANGES["ko"]) > 0:
        return "ko"
    if _count_in_ranges(text, _LANG_RANGES["ja"]) > 0:
        return "ja"
    if _count_in_ranges(text, _LANG_RANGES["zh-CN"]) > 0:
        # 区分简体/繁体中文：繁体特有字符占比高则判为 zh-TW
        trad_count = sum(1 for ch in text if ch in _TRADITIONAL_CHARS)
        return "zh-TW" if trad_count > 0 else "zh-CN"
    if _count_in_ranges(text, _LANG_RANGES["uk"]) > 0:
        return "uk"

    # 2) 拉丁系：含重音字符 -> es，否则 en
    if _count_in_ranges(text, _LANG_RANGES["es"]) > 0:
        return "es"
    return "en"


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


# 语言代码 -> 该语言下 Prompt 的本地化文案。
# 每个语言条目包含：turn 名称、无历史/无合法走法占位、吐槽指令、示例、JSON 说明。
# 关键：明确告知模型当前界面语言，要求 message 必须用该语言生成。
_LANG_PROMPTS = {
    "zh-CN": {
        "turn": ("白方", "黑方"),
        "none": "(无)",
        "lang_instruction": "你必须用简体中文生成 message。",
        "system": "你是一个国际象棋引擎，只负责输出下一步棋的走法，以及一句符合你性格的简短吐槽。",
        "board": "下面是当前棋盘（大写=白方，小写=黑方，. =空格）：",
        "turn_line": "当前轮到：{turn}（{side}）",
        "history": "已走的棋步：{history}",
        "legal": "当前方（{turn}）所有合法走法如下：",
        "choose": "请从上面的合法走法列表中，选择你认为最好的一步棋。",
        "important": "【重要】你只能从上面列出的合法走法中选择，绝不能输出列表之外的走法。",
        "feedback": "\n【上次你选错了】你上次输出了 {last}，但它不是合法走法。\n请务必从上面的合法走法列表中重新选择一个不同的走法。\n",
        "chat": "同时，根据当前棋局、你刚走的这步棋、以及对手的行动，用一句简短的话吐槽/调侃/阴阳怪气一下（30 个中文字符以内）。",
        "examples": "语气示例（只是示例，不要照抄）：\n“哈哈哈哈！你马炸了！”\n“不是哥们，你这一步认真的？”\n“好好好，这么玩是吧。”\n“完了，你的王要遭罪了。”\n“漂亮，不过你真的确定要这么走？”\n“哈哈，终于让我逮到了。”",
        "chat_req": "要求：必须和当前棋局相关；不要每次用相同句式；不要长篇大论；不要输出思考过程；不要解释分析；不要为了凑字数强行说话。",
        "json": "你的整个回复必须严格遵循以下 JSON 格式，不要输出任何其他内容：",
        "json_example": '{"move": "e7e5", "message": "哈哈，你这一步有点东西。"}',
        "json_note": "其中 move 是标准 UCI 走法（如 e2e4、g1f3、e1g1、e7e8q），必须从上面的合法走法列表中选择；message 是你的一句话。",
        "no_extra": "禁止输出 Markdown、代码块、解释、多个候选走法或任何其他字段。",
    },
    "zh-TW": {
        "turn": ("白方", "黑方"),
        "none": "(無)",
        "lang_instruction": "你必須用繁體中文生成 message。",
        "system": "你是一個國際象棋引擎，只負責輸出下一步棋的走法，以及一句符合你性格的簡短吐槽。",
        "board": "下面是當前棋盤（大寫=白方，小寫=黑方，. =空格）：",
        "turn_line": "當前輪到：{turn}（{side}）",
        "history": "已走的棋步：{history}",
        "legal": "當前方（{turn}）所有合法走法如下：",
        "choose": "請從上面的合法走法列表中，選擇你認為最好的一步棋。",
        "important": "【重要】你只能從上面列出的合法走法中選擇，絕不能輸出列表之外的走法。",
        "feedback": "\n【上次你選錯了】你上次輸出了 {last}，但它不是合法走法。\n請務必從上面的合法走法列表中重新選擇一個不同的走法。\n",
        "chat": "同時，根據當前棋局、你剛走的這步棋、以及對手的行動，用一句簡短的話吐槽/調侃/陰陽怪氣一下（30 個中文字元以內）。",
        "examples": "語氣示例（只是示例，不要照抄）：\n“哈哈哈哈！你馬炸了！”\n“不是哥們，你這一步認真的？”\n“好好好，這麼玩是吧。”\n“完了，你的王要遭罪了。”\n“漂亮，不過你真的確定要這麼走？”\n“哈哈，終於讓我逮到了。”",
        "chat_req": "要求：必須和當前棋局相關；不要每次用相同句式；不要長篇大論；不要輸出思考過程；不要解釋分析；不要為了湊字數強行說話。",
        "json": "你的整個回覆必須嚴格遵循以下 JSON 格式，不要輸出任何其他內容：",
        "json_example": '{"move": "e7e5", "message": "哈哈，你這一步有點東西。"}',
        "json_note": "其中 move 是標準 UCI 走法（如 e2e4、g1f3、e1g1、e7e8q），必須從上面的合法走法列表中選擇；message 是你的一句話。",
        "no_extra": "禁止輸出 Markdown、程式碼區塊、解釋、多個候選走法或任何其他欄位。",
    },
    "en": {
        "turn": ("White", "Black"),
        "none": "(none)",
        "lang_instruction": "You MUST generate the message in English.",
        "system": "You are a chess engine. Your only job is to output the next move and one short, in-character quip.",
        "board": "Here is the current board (uppercase=White, lowercase=Black, .=empty):",
        "turn_line": "Turn: {turn} ({side})",
        "history": "Moves played: {history}",
        "legal": "All legal moves for {turn}:",
        "choose": "Pick the move you think is best from the legal move list above.",
        "important": "[IMPORTANT] You may ONLY choose from the legal moves listed above. Never output a move outside that list.",
        "feedback": "\n[You were wrong last time] You previously output {last}, which is not a legal move.\nPlease choose a different move from the legal move list above.\n",
        "chat": "Also, based on the current position, the move you just made, and your opponent's actions, add one short witty/trash-talking remark (30 characters or fewer).",
        "examples": "Tone examples (just examples, do not copy):\n\"Ha! Your knight is toast!\"\n\"Bro, are you serious with that move?\"\n\"Oh nice, so that's how we're playing.\"\n\"RIP, your king is in trouble.\"\n\"Nice, but are you really sure about that?\"\n\"Ha, finally got you.\"",
        "chat_req": "Requirements: must relate to the current game; don't reuse the same phrasing every time; keep it short; don't output your thought process; don't explain or analyze; don't force words just to fill space.",
        "json": "Your entire reply must strictly follow this JSON format and nothing else:",
        "json_example": '{"move": "e7e5", "message": "Ha, that move of yours is something."}',
        "json_note": "move is a standard UCI move (e.g. e2e4, g1f3, e1g1, e7e8q) and must be chosen from the legal move list above; message is your one-liner.",
        "no_extra": "No Markdown, no code blocks, no explanations, no multiple candidate moves, and no other fields.",
    },
    "ja": {
        "turn": ("白", "黒"),
        "none": "(なし)",
        "lang_instruction": "message は必ず日本語で生成してください。",
        "system": "あなたはチェスエンジンです。次の一手と、あなたの性格に合った短い一言だけを出力してください。",
        "board": "現在の盤面です（大文字=白、小文字=黒、.=空きマス）：",
        "turn_line": "手番：{turn}（{side}）",
        "history": "指した手：{history}",
        "legal": "{turn} の合法手は次のとおりです：",
        "choose": "上の合法手リストから、あなたが最善だと思う手を選んでください。",
        "important": "【重要】上の合法手リストからだけ選んでください。リスト外の手を絶対に出力しないでください。",
        "feedback": "\n【前回は間違いでした】あなたは前回 {last} を出力しましたが、それは合法手ではありません。\n上の合法手リストから別の手を選び直してください。\n",
        "chat": "また、現在の局面・あなたが指した手・相手の動きに基づいて、短い一言で皮肉や煽りを入れてください（30文字以内）。",
        "examples": "口調の例（あくまで例なので、そのまま真似しないでください）：\n「ははっ！お前のナイトは終わりだ！」\n「おい、その手本気か？」\n「へえ、そうやって遊ぶのか。」\n「終わったな、お前のキングは危ないぞ。」\n「いいね、でも本当にそれでいいの？」\n「はは、ようやく捕まえた。」",
        "chat_req": "条件：現在の対局に関連させること；毎回同じ言い回しにしないこと；短くすること；思考過程を出力しないこと；説明や分析をしないこと；字数稼ぎで無理に話さないこと。",
        "json": "あなたの返答全体は、次の JSON 形式に厳密に従い、それ以外は何も出力しないでください：",
        "json_example": '{"move": "e7e5", "message": "はは、その手はなかなかだな。"}',
        "json_note": "move は標準 UCI 手（例：e2e4、g1f3、e1g1、e7e8q）で、上の合法手リストから選ぶこと；message はあなたの一言。",
        "no_extra": "Markdown、コードブロック、説明、複数の候補手、その他のフィールドは禁止です。",
    },
    "es": {
        "turn": ("Blancas", "Negras"),
        "none": "(ninguna)",
        "lang_instruction": "Debes generar el message en español.",
        "system": "Eres un motor de ajedrez. Tu única tarea es dar el siguiente movimiento y una breve frase con tu personalidad.",
        "board": "Este es el tablero actual (mayúsculas=Blancas, minúsculas=Negras, .=vacío):",
        "turn_line": "Turno: {turn} ({side})",
        "history": "Movimientos jugados: {history}",
        "legal": "Todos los movimientos legales para {turn}:",
        "choose": "Elige el movimiento que creas mejor de la lista de movimientos legales de arriba.",
        "important": "[IMPORTANTE] Solo puedes elegir de los movimientos legales listados arriba. Nunca salgas de esa lista.",
        "feedback": "\n[Te equivocaste la última vez] Antes sacaste {last}, que no es un movimiento legal.\nPor favor elige un movimiento diferente de la lista de arriba.\n",
        "chat": "Además, según la posición actual, el movimiento que acabas de hacer y las acciones del rival, añade un comentario corto y con chispa (30 caracteres o menos).",
        "examples": "Ejemplos de tono (solo ejemplos, no los copies):\n\"¡Ja! ¡Tu caballo está perdido!\"\n\"Tío, ¿en serio ese movimiento?\"\n\"Vale, vale, así jugamos.\"\n\"Se acabó, tu rey está en problemas.\"\n\"Bonito, pero ¿seguro que quieres eso?\"\n\"Ja, por fin te tengo.\"",
        "chat_req": "Requisitos: debe relacionarse con la partida; no repitas la misma frase cada vez; sé breve; no muestres tu proceso de pensamiento; no expliques ni analices; no fuerces palabras solo para llenar.",
        "json": "Toda tu respuesta debe seguir estrictamente este formato JSON y nada más:",
        "json_example": '{"move": "e7e5", "message": "Ja, ese movimiento tuyo tiene miga."}',
        "json_note": "move es un movimiento UCI estándar (p. ej. e2e4, g1f3, e1g1, e7e8q) y debe elegirse de la lista de arriba; message es tu frase.",
        "no_extra": "Prohibido Markdown, bloques de código, explicaciones, múltiples movimientos candidatos o cualquier otro campo.",
    },
    "uk": {
        "turn": ("Білі", "Чорні"),
        "none": "(немає)",
        "lang_instruction": "Ти обов'язково маєш генерувати message українською мовою.",
        "system": "Ти — шаховий двигун. Твоє завдання — лише назвати наступний хід і одну коротку фразу в твоєму стилі.",
        "board": "Ось поточна дошка (великі=Білі, малі=Чорні, .=порожньо):",
        "turn_line": "Хід: {turn} ({side})",
        "history": "Зроблені ходи: {history}",
        "legal": "Усі легальні ходи для {turn}:",
        "choose": "Обери хід, який вважаєш найкращим, зі списку легальних ходів вище.",
        "important": "[ВАЖЛИВО] Ти можеш обирати лише з легальних ходів, перелічених вище. Ніколи не виходь за межі цього списку.",
        "feedback": "\n[Минулого разу ти помилився] Ти раніше вивів {last}, але це не легальний хід.\nБудь ласка, обери інший хід зі списку вище.\n",
        "chat": "Також, з огляду на поточну позицію, твій щойно зроблений хід і дії суперника, додай одну коротку дотепну/підколюючу фразу (до 30 символів).",
        "examples": "Приклади тону (лише приклади, не копіюй):\n\"Ха! Твій кінь готовий!\"\n\"Брате, ти серйозно з таким ходом?\"\n\"Ну добре, так і граємо.\"\n\"Все, твій король у біді.\"\n\"Гарно, але ти точно впевнений?\"\n\"Ха, нарешті я тебе спіймав.\"",
        "chat_req": "Вимоги: має стосуватися поточної гри; не повторюй ту саму фразу щоразу; будь коротким; не показуй процес мислення; не пояснюй і не аналізуй; не вигадуй слова просто щоб заповнити місце.",
        "json": "Уся твоя відповідь має суворо відповідати цьому JSON-формату і нічому більше:",
        "json_example": '{"move": "e7e5", "message": "Ха, цей твій хід щось та й вартий."}',
        "json_note": "move — це стандартний UCI-хід (напр. e2e4, g1f3, e1g1, e7e8q), який треба обрати зі списку вище; message — твоя фраза.",
        "no_extra": "Заборонено Markdown, блоки коду, пояснення, кілька кандидатів або будь-які інші поля.",
    },
    "ko": {
        "turn": ("백", "흑"),
        "none": "(없음)",
        "lang_instruction": "message는 반드시 한국어로 생성해야 합니다.",
        "system": "당신은 체스 엔진입니다. 다음 수와 당신의 성격에 맞는 짧은 한마디만 출력하세요.",
        "board": "현재 보드입니다 (대문자=백, 소문자=흑, .=빈 칸):",
        "turn_line": "차례: {turn} ({side})",
        "history": "둔 수: {history}",
        "legal": "{turn}의 모든 합법 수는 다음과 같습니다:",
        "choose": "위 합법 수 목록에서 가장 좋다고 생각하는 수를 선택하세요.",
        "important": "[중요] 위에 나열된 합법 수에서만 선택하세요. 목록 밖의 수는 절대 출력하지 마세요.",
        "feedback": "\n[지난번에 틀렸습니다] 당신은 이전에 {last}를 출력했지만, 그것은 합법 수가 아닙니다.\n위 합법 수 목록에서 다른 수를 다시 선택하세요.\n",
        "chat": "또한 현재 상황, 방금 둔 수, 상대의 움직임을 바탕으로 짧은 한마디로 비꼬거나 조롱하세요 (30자 이내).",
        "examples": "말투 예시 (예시일 뿐, 그대로 따라 하지 마세요):\n\"하하! 네 나이트는 끝났다!\"\n\"야, 그 수 진심이야?\"\n\"그래그래, 그렇게 두는 거지.\"\n\"끝났다, 네 킹이 위험해.\"\n\"좋아, 근데 정말 그걸 둘 거야?\"\n\"하하, 드디어 잡았다.\"",
        "chat_req": "조건: 현재 대국과 관련되어야 함; 매번 같은 표현을 쓰지 말 것; 짧게 할 것; 생각 과정을 출력하지 말 것; 설명이나 분석을 하지 말 것; 글자 수를 채우려고 억지로 말하지 말 것.",
        "json": "당신의 전체 답변은 다음 JSON 형식을 엄격히 따라야 하며, 그 외에는 아무것도 출력하지 마세요:",
        "json_example": '{"move": "e7e5", "message": "하하, 네 그 수 좀 있네."}',
        "json_note": "move는 표준 UCI 수(예: e2e4, g1f3, e1g1, e7e8q)이며 위 합법 수 목록에서 선택해야 함; message는 당신의 한마디.",
        "no_extra": "Markdown, 코드 블록, 설명, 여러 후보 수 또는 다른 필드는 금지입니다.",
    },
}


def _build_prompt(fen: str, turn: str, move_history: list, legal_moves: list,
                  last_error: str = "", language: str = "en") -> str:
    """构造发送给 AI 的 Prompt。

    明确要求模型只返回一个标准 UCI 走法，不返回解释/Markdown/多个候选。
    附带 ASCII 棋盘图，帮助模型准确理解当前局面。
    关键：把当前方所有合法走法列出来，要求模型从中选择一个，杜绝非法走法。
    若上次选错（last_error 非空），明确告知 AI 上次选错了，要求重新选一个不同的。

    language: 当前界面语言代码（zh-CN/zh-TW/en/ja/es/uk/ko）。
    聊天内容（message）必须用该语言生成；不支持的代码回退到 en。
    """
    # 选择语言文案；不支持的代码回退到 en
    lang = _LANG_PROMPTS.get(language) or _LANG_PROMPTS["en"]
    turn_name = lang["turn"][0] if turn == "white" else lang["turn"][1]

    history = " ".join(move_history) if move_history else lang["none"]
    board = fen_to_ascii(fen)
    legal = ", ".join(legal_moves) if legal_moves else lang["none"]

    feedback = ""
    if last_error:
        feedback = lang["feedback"].format(last=last_error)

    return (
        lang["system"] + "\n"
        + lang["board"] + "\n"
        + f"{board}\n\n"
        + lang["turn_line"].format(turn=turn_name, side=turn) + "\n"
        + lang["history"].format(history=history) + "\n\n"
        + lang["legal"].format(turn=turn_name) + "\n"
        + f"{legal}\n\n"
        + lang["choose"] + "\n"
        + lang["important"] + "\n"
        + feedback
        + lang["chat"] + "\n"
        + lang["examples"] + "\n"
        + lang["chat_req"] + "\n"
        + lang["json"] + "\n"
        + lang["json_example"] + "\n"
        + lang["json_note"] + "\n"
        + lang["no_extra"]
    )


def _extract_message_from_raw(raw: str) -> str:
    """从 AI 原始回复中提取 message 字段（聊天内容）。

    优先解析 JSON（{"move": "...", "message": "..."}）；
    否则从纯文本中剥离 UCI 走法片段，返回剩余自然语言。
    """
    if not raw:
        return ""
    text = raw.strip()
    # 尝试解析 JSON
    try:
        obj = json.loads(text)
        if isinstance(obj, dict):
            msg = str(obj.get("message", "")).strip()
            if msg:
                return msg
    except (ValueError, TypeError):
        pass
    # 从文本中提取 {...} 片段
    for m in re.finditer(r"\{[^{}]*\}", text):
        try:
            obj = json.loads(m.group(0))
            if isinstance(obj, dict):
                msg = str(obj.get("message", "")).strip()
                if msg:
                    return msg
        except (ValueError, TypeError):
            continue
    # 纯文本：剥离 UCI 走法片段
    cleaned = re.sub(r"[a-h][1-8]-?[a-h][1-8][qrbn]?", "", text)
    cleaned = cleaned.replace("{", "").replace("}", "").replace('"', "")
    cleaned = cleaned.replace("move", "").replace("message", "").replace(":", "")
    cleaned = cleaned.strip(" ,。.!！?？;；")
    return cleaned


def _call_api(url: str, api_key: str, payload: dict, timeout: float) -> str:
    """发送一次 Chat Completions 请求，返回 AI 原始回复文本。"""
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

    try:
        obj = json.loads(body)
        content = obj["choices"][0]["message"]["content"]
    except (KeyError, IndexError, json.JSONDecodeError) as e:
        raise OpenAICompatibleError("API 响应格式无效") from e
    return content


def request_move(base_url: str, api_key: str, model: str,
                 fen: str, turn: str, move_history: list,
                 legal_moves: list = None,
                 last_error: str = "",
                 language: str = "en",
                 timeout: float = 60.0,
                 max_retries: int = 3) -> str:
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
        language: 当前界面语言代码（zh-CN/zh-TW/en/ja/es/uk/ko）。
                  聊天内容（message）必须用该语言生成。
        timeout:  请求超时（秒）
        max_retries: 语言不匹配时的最大重试次数（含首次请求）。

    返回：
        AI 的原始文本回复。

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

    lang = _LANG_PROMPTS.get(language) or _LANG_PROMPTS["en"]
    # 语言指令用目标语言本身书写，模型更易遵循（避免中文指令对韩语等模型失效）。
    base_system = (
        lang["system"]
        + " " + lang["lang_instruction"]
        + " (Current UI language: " + language + ")"
    )

    # 重试时追加的更强语言指令（用目标语言书写，强调必须用该语言）
    retry_instruction = lang.get("lang_instruction", "")
    if not retry_instruction:
        retry_instruction = "You MUST generate the message in " + language + "."

    last_content = ""
    for attempt in range(max_retries):
        # 每次重试追加更强的语言指令
        system_msg = base_system
        if attempt > 0:
            system_msg += (
                "\n\n[IMPORTANT RETRY] 你上一次的 message 语言不对。"
                " 必须严格使用界面语言 " + language + " 生成 message。"
                " " + retry_instruction
            )

        payload = {
            "model": model,
            "messages": [
                {"role": "system", "content": system_msg},
                {"role": "user", "content": _build_prompt(fen, turn, move_history, legal_moves or [], last_error, language)},
            ],
            "temperature": 0.2,
            "max_tokens": 32,
        }

        last_content = _call_api(url, api_key, payload, timeout)

        # 检测 message 语言是否匹配界面语言
        message = _extract_message_from_raw(last_content)
        if not message:
            # 无 message（纯走法），无需语言校验，直接返回
            return last_content
        detected = detect_language(message)
        if detected == language:
            return last_content
        # 语言不匹配，继续重试调教

    # 达到最大重试次数仍未匹配，返回最后一次结果
    return last_content
