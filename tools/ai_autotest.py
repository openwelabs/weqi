#!/usr/bin/env python3
"""AI 走法自动化测试脚本。

模拟 C++ 的完整 AI 对局流程：
1. 从初始局面开始
2. 每回合调用 AI（request_move）获取走法
3. 用 C++ 验证器（/tmp/ai_move_validator）检查走法是否合法
4. 合法则应用走法（验证器返回新 FEN），继续下一回合
5. 非法则重试（最多 3 次），记录统计

用法：
    python3 tools/ai_autotest.py [回合数] [--config 配置文件路径]

不包含任何 API Key 硬编码；从 Weqi 的 ai_providers.json 读取。
"""

import json
import os
import subprocess
import sys

# 允许从仓库根目录运行
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "ai_adapter"))

from parser import parse_ai_response
from providers.openai_compatible import request_move, OpenAICompatibleError

VALIDATOR = "/tmp/ai_move_validator"
DEFAULT_CONFIG = os.path.expanduser("~/.local/share/Weqi/Weqi/config/ai_providers.json")
INITIAL_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"


def load_provider(config_path):
    """从配置文件读取第一个 provider。"""
    with open(config_path, encoding="utf-8") as f:
        providers = json.load(f)
    if not providers:
        print("错误：配置文件中没有 provider")
        sys.exit(1)
    return providers[0]


def validate_move(fen, move):
    """调用 C++ 验证器检查走法是否合法。返回 (legal, new_fen, reason)。"""
    payload = json.dumps({"fen": fen, "move": move})
    try:
        proc = subprocess.run(
            [VALIDATOR], input=payload, capture_output=True, text=True, timeout=10
        )
        result = json.loads(proc.stdout.strip())
        return result.get("legal", False), result.get("new_fen"), result.get("reason", "")
    except Exception as e:
        return False, None, f"validator_error: {e}"


def list_legal_moves(fen):
    """调用 C++ 验证器获取当前方所有合法走法（UCI 格式）。"""
    payload = json.dumps({"fen": fen, "list_legal": True})
    try:
        proc = subprocess.run(
            [VALIDATOR], input=payload, capture_output=True, text=True, timeout=10
        )
        result = json.loads(proc.stdout.strip())
        return result.get("legal_moves", [])
    except Exception as e:
        print(f"  [错误] 获取合法走法失败: {e}")
        return []


def fen_turn(fen):
    """从 FEN 提取当前回合。"""
    return fen.split(" ")[1]


def main():
    max_moves = int(sys.argv[1]) if len(sys.argv) > 1 else 20
    config_path = DEFAULT_CONFIG
    if "--config" in sys.argv:
        idx = sys.argv.index("--config")
        config_path = sys.argv[idx + 1]

    provider = load_provider(config_path)
    print(f"Provider: {provider['name']} ({provider['model']})")
    print(f"Base URL: {provider['baseUrl']}")
    print(f"测试回合数: {max_moves}")
    print("=" * 60)

    fen = INITIAL_FEN
    move_history = []
    stats = {"legal": 0, "illegal": 0, "retries": 0, "failed_requests": 0}
    illegal_moves = []

    for ply in range(1, max_moves + 1):
        turn = fen_turn(fen)
        print(f"\n--- 第 {ply} 步（{turn}）---")
        print(f"FEN: {fen}")

        # 获取当前方所有合法走法，传给 AI 让其从中选择
        legal_moves = list_legal_moves(fen)
        if not legal_moves:
            print("  [失败] 当前方无合法走法（对局结束）")
            break
        print(f"  合法走法数: {len(legal_moves)}")

        # 请求 AI 走法，最多重试 3 次
        move = None
        for attempt in range(1, 4):
            try:
                raw = request_move(
                    provider["baseUrl"], provider["apiKey"], provider["model"],
                    fen, turn, move_history, legal_moves, timeout=60.0,
                )
            except OpenAICompatibleError as e:
                print(f"  [请求失败] {e}")
                stats["failed_requests"] += 1
                break

            result = parse_ai_response(raw)
            if not result.get("ok"):
                print(f"  [解析失败] raw={raw!r}")
                stats["illegal"] += 1
                illegal_moves.append((ply, turn, raw, "parse_failed"))
                continue

            move = result["move"]
            # 关键：AI 返回的走法必须从合法列表中选择
            if move not in legal_moves:
                print(f"  AI 走法: {move}  ✗ 不在合法列表中")
                stats["illegal"] += 1
                stats["retries"] += 1
                illegal_moves.append((ply, turn, move, "not_in_legal_list"))
                move = None
                continue

            legal, new_fen, reason = validate_move(fen, move)
            if legal:
                print(f"  AI 走法: {move}  ✓ 合法")
                stats["legal"] += 1
                fen = new_fen
                move_history.append(move)
                break
            else:
                print(f"  AI 走法: {move}  ✗ 非法 ({reason})")
                stats["illegal"] += 1
                stats["retries"] += 1
                illegal_moves.append((ply, turn, move, reason))
                move = None
        else:
            print(f"  [失败] 3 次重试后仍无合法走法，对局终止")
            break

        if move is None:
            print(f"  [失败] 无法获得合法走法，对局终止")
            break

    print("\n" + "=" * 60)
    print("测试结果统计：")
    print(f"  合法走法: {stats['legal']}")
    print(f"  非法走法: {stats['illegal']}")
    print(f"  重试次数: {stats['retries']}")
    print(f"  请求失败: {stats['failed_requests']}")
    if illegal_moves:
        print("\n非法走法明细：")
        for ply, turn, move, reason in illegal_moves:
            print(f"  第{ply}步({turn}): {move!r} -> {reason}")
    print(f"\n最终 FEN: {fen}")
    print(f"走法历史: {' '.join(move_history)}")


if __name__ == "__main__":
    main()
