#!/usr/bin/env python3
"""Weqi AI Adapter 入口。

C++ 通过 QProcess 启动本脚本，从 stdin 读取 JSON 请求，
调用第三方 AI API，解析出 UCI 走法，将结果以 JSON 写入 stdout。

请求格式（stdin）：
{
    "action": "get_ai_move",
    "provider": {"base_url": "...", "api_key": "...", "model": "..."},
    "game": {"fen": "...", "turn": "white|black", "move_history": [...]}
}

响应格式（stdout）：
成功：{"ok": true, "move": "e2e4"}
失败：{"ok": false, "error": "..."}

本脚本只负责 AI 通信与走法提取，不判断走法合法性（由 C++ 负责）。
"""

import json
import os
import sys
import traceback

from parser import parse_ai_response
from providers.openai_compatible import request_move, OpenAICompatibleError

# 调试日志：记录 AI 返回的原始文本，便于排查"无效走法"问题。
# 写入 /tmp/weqi_ai_debug.log，不包含 API Key。
_DEBUG_LOG = "/tmp/weqi_ai_debug.log"


def _log_debug(model: str, fen: str, turn: str, history: list, raw: str, result: dict) -> None:
    """把 AI 原始回复与解析结果写入调试日志（不含 API Key）。"""
    try:
        with open(_DEBUG_LOG, "a", encoding="utf-8") as f:
            f.write("=== model=%s turn=%s ===\n" % (model, turn))
            f.write("fen: %s\n" % fen)
            f.write("history: %s\n" % " ".join(history))
            f.write("raw: %r\n" % raw)
            f.write("result: %s\n" % json.dumps(result, ensure_ascii=False))
            f.write("\n")
    except Exception:
        pass


def _read_request() -> dict:
    """从 stdin 读取一行 JSON 请求。"""
    line = sys.stdin.readline()
    if not line:
        raise ValueError("stdin 无输入")
    return json.loads(line)


def _write_response(obj: dict) -> None:
    """将 JSON 响应写入 stdout 并刷新。"""
    sys.stdout.write(json.dumps(obj))
    sys.stdout.write("\n")
    sys.stdout.flush()


def main() -> int:
    try:
        req = _read_request()
    except Exception as e:
        _write_response({"ok": False, "error": f"invalid_request: {e}"})
        return 1

    action = req.get("action")
    if action != "get_ai_move":
        _write_response({"ok": False, "error": f"unknown_action: {action}"})
        return 1

    provider = req.get("provider") or {}
    game = req.get("game") or {}

    base_url = provider.get("base_url", "")
    api_key = provider.get("api_key", "")
    model = provider.get("model", "")
    fen = game.get("fen", "")
    turn = game.get("turn", "white")
    move_history = game.get("move_history", []) or []
    legal_moves = game.get("legal_moves", []) or []
    last_error = game.get("last_error", "") or ""
    language = game.get("language", "en") or "en"

    if not fen:
        _write_response({"ok": False, "error": "missing_fen"})
        return 1

    try:
        raw = request_move(base_url, api_key, model, fen, turn, move_history,
                           legal_moves, last_error=last_error, language=language)
    except OpenAICompatibleError as e:
        _write_response({"ok": False, "error": str(e)})
        return 1
    except Exception as e:
        _write_response({"ok": False, "error": f"adapter_error: {e}"})
        return 1

    # 解析 AI 回复，提取 UCI 走法
    result = parse_ai_response(raw)
    _log_debug(model, fen, turn, move_history, raw, result)
    _write_response(result)
    return 0 if result.get("ok") else 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception:
        # 兜底：任何未捕获异常都输出 JSON 错误，避免 C++ 解析失败
        _write_response({"ok": False, "error": "adapter_crash"})
        sys.exit(1)
