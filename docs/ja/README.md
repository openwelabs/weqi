# Weqi

**Weqi** は、モダンでクリーンなオープンソースのデスクトップチェスアプリケーションです。すべてのチェスのルールはローカルの C++ エンジンで実装されており、対人対局、人機対局、AI 同士の対局、棋譜の再生に対応しています。

> **このドキュメントを読む言語を選択：**
> [English](../../README.md) · [简体中文](../zh-CN/README.md) · [繁體中文](../zh-TW/README.md) · [Español](../es/README.md) · [Українська](../uk/README.md) · [한국어](../ko/README.md)

---

## 機能

- **4 つの対局モード**
  - **対人対局** — 同じ端末で 2 人が対局。
  - **人機対局** — 選択した AI プロバイダーと対局。
  - **AI 対局** — 2 つの AI が自動で対局（あなたの戦績にはカウントされません）。
  - **棋譜の再生** — 過去の対局を振り返る。
- **完全なチェスのルール** をローカル C++ で実装（合法手の検証、チェック、チェックメイト、ステイルメイト、キャスリング、アンパッサン、プロモーションなど）。
- **プレイヤープロフィールとレーティング** — レーティング、最高レーティングを追跡し、プレイヤー名を編集できます。
- **戦績統計** — 対局数、勝率、勝ち、引き分け、負け、最高連勝、現在の連勝、最高レーティング。
- **対局履歴** — 日付、モード、対戦相手、結果、レーティング変動を含む過去の対局を閲覧。
- **未完了の対局を続行** — 進行中の対局をいつでも再開。
- **AI プロバイダー** — AI プロバイダーを追加・編集・削除（名前、タイプ、Base URL、API キー、モデル）。API キーはシステムのユーザーデータディレクトリにのみ保存され、プロジェクトには書き込まれません。
- **AI チャット** — AI が自分の手について短いコメントを表示し、メッセージは UI 言語に従います。
- **国際化（i18n）** — 7 つの UI 言語：簡体字中国語、繁体字中国語、英語、日本語、スペイン語、ウクライナ語、韓国語。再起動せずに実行時に言語を切り替えられます。

## 技術スタック

- **C++17**
- **Qt 6**（Widgets）
- **CMake**

## ディレクトリ構成

```
weqi/
├── CMakeLists.txt
├── README.md
├── ai_adapter/          # Python AI アダプター（言語検出、リトライ/調整）
│   ├── main.py
│   ├── parser.py
│   └── providers/
│       └── openai_compatible.py
├── dist/                # 配布用ディレクトリ（コンパイル済みバイナリ + パッケージングスクリプト）
├── resources/           # Qt リソースファイル
├── scripts/             # 補助スクリプト
├── src/
│   ├── main.cpp         # プログラムのエントリポイント
│   ├── MainWindow.h/.cpp
│   ├── GameController.h/.cpp
│   ├── ChessBoard.h/.cpp
│   ├── ChessPiece.h/.cpp
│   ├── ai/              # AI マネージャー
│   ├── data/            # 設定、言語、プロフィール、戦績、履歴マネージャー
│   └── pages/           # ホーム、新規対局、対局、人機、AI 対局、履歴、設定、バージョン情報
├── tests/               # C++ テスト
├── tools/               # 開発ツール
└── translations/        # Qt .ts 翻訳ファイル（7 言語）
```

## ビルド

### 依存関係

- CMake ≥ 3.16
- Qt 6（≥ 6.2、Widgets モジュールを含む）
- C++17 対応コンパイラ（GCC / Clang）
- Python 3（AI アダプター用）

### ビルド手順

```bash
# 1. 設定
cmake -S . -B build

# 2. ビルド
cmake --build build -j

# 3. 実行
./build/Weqi
```

またはワンライナーで：

```bash
cmake -S . -B build && cmake --build build -j && ./build/Weqi
```

### 翻訳

翻訳ファイルは `translations/` にあり、Qt の `qt_add_translations` によってバイナリにコンパイルされます。`tr()` 文字列を変更した後は翻訳を更新します：

```bash
# .ts ファイルを更新（新しい文字列を抽出）
lupdate src -ts translations/weqi_<lang>.ts

# .qm ファイルをリリース
lrelease translations/weqi_<lang>.ts
```

## パッケージング

`dist/` ディレクトリには、コンパイル済みバイナリと Python AI アダプターが含まれており、インストーラーにパッケージングできます。

```
dist/
├── Weqi                  # コンパイル済み C++ バイナリ（Qt6 Widgets、7 言語内蔵）
├── ai_adapter/           # Python AI アダプター
├── package-deb.sh        # .deb をビルド（Linux）
├── package-rpm.sh        # .rpm をビルド（Linux、Fedora/RHEL/openSUSE）
├── weqi.spec             # RPM spec ファイル
├── package-exe.sh        # .exe をビルド（Windows、Windows 上で実行）
├── package-appimage.sh   # .AppImage をビルド（Linux、任意のディストリビューション）
├── weqi.desktop          # AppImage デスクトップエントリ
├── weqi.png              # AppImage アイコン（512×512）
└── README.md
```

- **.deb**：`cd dist && ./package-deb.sh` → `weqi_0.1.0_amd64.deb`
- **.rpm**：`cd dist && ./package-rpm.sh` → `weqi-0.1.0-1.fc44.x86_64.rpm`
- **.exe**：Windows 上で `./package-exe.sh` を実行 → `weqi-win/` ディレクトリ（`windeployqt` を使用）
- **.AppImage**：`cd dist && ./package-appimage.sh` → `Weqi-0.1.0-x86_64.AppImage`（単一ファイル、インストール不要）

バイナリは実行ファイルからの相対パスで `ai_adapter/main.py` を探します。パッケージでインストールした場合は `/usr/share/weqi/ai_adapter/main.py` から読み込みます。

## 使い方

- **駒を選択**：任意の駒を左クリックしてハイライトします。
- **駒を移動**：選択後、目的のマスをクリックします。ローカルエンジンが合法手を検証します。
- **待った**：「待った」ボタンで一手戻します。
- **新しい対局**：対局ページまたはホーム画面から新しい対局を開始します。
- **AI 対局**：2 つの AI プロバイダーを選択し、「開始 / 一時停止 / 再開 / 停止」で自動対局を制御します。
- **言語の切り替え**：「設定 → 言語」を開き、7 つの対応言語のいずれかを選択します（または「システムに従う」）。

## AI モデルの追加

AI と対局するには、まず「設定 → AI Providers」で AI プロバイダーを追加します。各プロバイダーには 5 つの項目があります：

| 項目 | 入力内容 |
| --- | --- |
| **名称** | 自分で決める表示名。例：`DeepSeek`、`Qwen`、`豆包`。 |
| **提供商類型（プロバイダー種別）** | 下記のモデルはすべて OpenAI 互換 API を使用するため、`OpenAI Compatible` と入力します。 |
| **Base URL** | プロバイダーの API エンドポイント（下表参照）。 |
| **API Key** | プロバイダーのコンソールで発行した秘密キー。ローカルのユーザーデータディレクトリにのみ保存され、プロジェクトには書き込まれません。 |
| **模型（モデル）** | 正確なモデル名（下表参照）。 |

### 主要モデルの推奨設定

下記のプロバイダーはすべて OpenAI 互換エンドポイントを提供しているため、「プロバイダー種別」はすべて `OpenAI Compatible` と入力します。

| プロバイダー | Base URL | モデル（例） | API Key の発行場所 |
| --- | --- | --- | --- |
| **DeepSeek** | `https://api.deepseek.com/v1` | `deepseek-chat` | platform.deepseek.com |
| **Qwen（通義千問）** | `https://dashscope.aliyuncs.com/compatible-mode/v1` | `qwen-plus` | bailian.console.aliyun.com |
| **豆包（Doubao）** | `https://ark.cn-beijing.volces.com/api/v3` | `doubao-1-5-pro-32k-250115` | console.volcengine.com/ark |
| **ChatGPT（OpenAI）** | `https://api.openai.com/v1` | `gpt-4o` | platform.openai.com |
| **元宝（Tencent Hunyuan）** | `https://api.hunyuan.cloud.tencent.com/v1` | `hunyuan-turbo` | console.cloud.tencent.com/hunyuan |
| **Gemini（Google）** | `https://generativelanguage.googleapis.com/v1beta/openai` | `gemini-2.0-flash` | aistudio.google.com |
| **MiniMax** | `https://api.minimax.chat/v1` | `MiniMax-Text-01` | platform.minimaxi.com |
| **Kimi（Moonshot）** | `https://api.moonshot.cn/v1` | `moonshot-v1-8k` | platform.moonshot.cn |
| **Mimo（Xiaomi）** | `https://api.mimo.ai/v1` | `mimo-1` | platform.mimo.ai |
| **Claude（Anthropic）** | `https://api.anthropic.com/v1` | `claude-sonnet-4-20250514` | console.anthropic.com |
| **Grok（xAI）** | `https://api.x.ai/v1` | `grok-2-latest` | console.x.ai |

> **注意**：モデル名やエンドポイントは頻繁に変更されます。最新のモデル ID と Base URL は各プロバイダーの公式ドキュメントを確認してください。「名称」は自分で付けるラベルに過ぎず、実際に呼び出す API には影響しません。

## データ保存

- **設定とプロフィール**：システムのユーザーデータディレクトリに保存（Linux では `~/.local/share/Weqi/` など）。
- **AI プロバイダー設定**：ユーザーデータディレクトリに別途保存され、プロジェクトには書き込まれません。
- **対局履歴とセーブデータ**：ユーザーデータディレクトリに保存。

## ライセンス

オープンソース。詳細はリポジトリを参照してください。
