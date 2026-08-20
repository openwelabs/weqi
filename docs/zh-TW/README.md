# Weqi

**Weqi** 是一款現代、簡潔的開源桌面西洋棋應用。所有棋規均在本地 C++ 引擎中實現，支援真人對戰、人機對戰、AI 對戰與覆盤。

> **選擇語言閱讀本文檔：**
> [English](../../README.md) · [简体中文](../zh-CN/README.md) · [日本語](../ja/README.md) · [Español](../es/README.md) · [Українська](../uk/README.md) · [한국어](../ko/README.md)

---

## 功能特性

- **四種遊戲模式**
  - **真人對戰** — 同一裝置上雙人對弈。
  - **人機對戰** — 與所選 AI 提供商對弈。
  - **AI 對戰** — 兩個 AI 自動對弈（不計入你的戰績）。
  - **覆盤** — 回顧歷史對局。
- **完整棋規** 在本地 C++ 中實現（合法走法校驗、將軍、將死、逼和、王車易位、吃過路兵、升變等）。
- **玩家資料與 Rating** — 追蹤你的 Rating、最佳 Rating，並可編輯玩家名稱。
- **戰績統計** — 對局數、勝率、勝、和、負、最高連勝、當前連勝、最佳 Rating。
- **對局歷史** — 瀏覽過往對局，包含日期、模式、對手、結果與 Rating 變化。
- **繼續未完成對局** — 隨時恢復進行中的對局。
- **AI Providers** — 新增、編輯、刪除 AI 提供商（名稱、類型、Base URL、API Key、模型）。API Key 為私密資料，僅保存在系統使用者資料目錄，不會寫入專案。
- **AI 聊天** — AI 會對其走法做簡短點評，訊息語言跟隨介面語言。
- **國際化（i18n）** — 7 種介面語言：簡體中文、繁體中文、英語、日語、西班牙語、烏克蘭語、韓語。執行時切換語言，無需重啟。

## 技術棧

- **C++17**
- **Qt 6**（Widgets）
- **CMake**

## 目錄結構

```
weqi/
├── CMakeLists.txt
├── README.md
├── ai_adapter/          # Python AI 介面卡（語言偵測、重試/調教）
│   ├── main.py
│   ├── parser.py
│   └── providers/
│       └── openai_compatible.py
├── dist/                # 可分發目錄（編譯好的二進位 + 打包腳本）
├── resources/           # Qt 資源檔
├── scripts/             # 輔助腳本
├── src/
│   ├── main.cpp         # 程式進入點
│   ├── MainWindow.h/.cpp
│   ├── GameController.h/.cpp
│   ├── ChessBoard.h/.cpp
│   ├── ChessPiece.h/.cpp
│   ├── ai/              # AI 管理器
│   ├── data/            # 設定、語言、資料、戰績、歷史管理器
│   └── pages/           # 首頁、新對局、對局、人機、AI 對戰、歷史、設定、關於
├── tests/               # C++ 測試
├── tools/               # 開發工具
└── translations/        # Qt .ts 翻譯檔（7 種語言）
```

## 建置

### 依賴

- CMake ≥ 3.16
- Qt 6（≥ 6.2，含 Widgets 模組）
- 支援 C++17 的編譯器（GCC / Clang）
- Python 3（用於 AI 介面卡）

### 建置步驟

```bash
# 1. 設定
cmake -S . -B build

# 2. 編譯
cmake --build build -j

# 3. 執行
./build/Weqi
```

也可以使用一行命令：

```bash
cmake -S . -B build && cmake --build build -j && ./build/Weqi
```

### 翻譯

翻譯檔位於 `translations/`，透過 Qt 的 `qt_add_translations` 編譯進二進位檔。修改 `tr()` 字串後更新翻譯：

```bash
# 更新 .ts 檔（擷取新字串）
lupdate src -ts translations/weqi_<lang>.ts

# 發佈 .qm 檔
lrelease translations/weqi_<lang>.ts
```

## 打包

`dist/` 目錄包含編譯好的二進位與 Python AI 介面卡，可直接打包成安裝包。

```
dist/
├── Weqi                  # 編譯好的 C++ 二進位（Qt6 Widgets，內嵌 7 種語言）
├── ai_adapter/           # Python AI 介面卡
├── package-deb.sh        # 打包 .deb（Linux）
├── package-rpm.sh        # 打包 .rpm（Linux，Fedora/RHEL/openSUSE）
├── weqi.spec             # RPM spec 檔案
├── package-exe.sh        # 打包 .exe（Windows，需在 Windows 上執行）
├── package-appimage.sh   # 打包 .AppImage（Linux，任意發行版）
├── weqi.desktop          # AppImage 桌面入口
├── weqi.png              # AppImage 圖示（512×512）
└── README.md
```

- **.deb**：`cd dist && ./package-deb.sh` → `weqi_0.1.0_amd64.deb`
- **.rpm**：`cd dist && ./package-rpm.sh` → `weqi-0.1.0-1.fc44.x86_64.rpm`
- **.exe**：在 Windows 上執行 `./package-exe.sh` → `weqi-win/` 目錄（使用 `windeployqt`）
- **.AppImage**：`cd dist && ./package-appimage.sh` → `Weqi-0.1.0-x86_64.AppImage`（單一檔案，無需安裝）

二進位會相對於可執行檔查找 `ai_adapter/main.py`；透過安裝包安裝時，則從 `/usr/share/weqi/ai_adapter/main.py` 載入。

## 使用說明

- **選取棋子**：左鍵點擊任意棋子，該格會高亮。
- **移動棋子**：選取後點擊目標格，本地引擎會校驗走法合法性。
- **悔棋**：使用「悔棋」按鈕撤銷一步。
- **新遊戲**：在對局頁或首頁開始新對局。
- **AI 對戰**：選擇兩個 AI 提供商，使用「開始 / 暫停 / 繼續 / 停止」控制自動對局。
- **切換語言**：開啟「設定 → 語言」，選擇 7 種支援的語言之一（或「跟隨系統」）。

## 新增 AI 模型

要跟 AI 對弈，需要先在「設定 → AI Providers」中新增一個 AI 提供商。每個提供商有 5 個欄位：

| 欄位 | 填寫內容 |
| --- | --- |
| **名稱** | 你自己取的顯示名稱，例如 `DeepSeek`、`Qwen`、`豆包`。 |
| **提供商類型** | 下面這些模型都使用 OpenAI 相容介面，統一填 `OpenAI Compatible`。 |
| **Base URL** | 提供商的 API 位址（見下表）。 |
| **API Key** | 你在提供商控制台申請的私密金鑰，只保存在本地使用者資料目錄，不會寫入專案。 |
| **模型** | 精確的模型名稱（見下表）。 |

### 常見模型推薦設定

下面這些提供商都提供 OpenAI 相容介面，所以「提供商類型」全部填 `OpenAI Compatible`。

| 提供商 | Base URL | 模型（範例） | 申請 API Key 的地方 |
| --- | --- | --- | --- |
| **DeepSeek** | `https://api.deepseek.com/v1` | `deepseek-chat` | platform.deepseek.com |
| **Qwen（通義千問）** | `https://dashscope.aliyuncs.com/compatible-mode/v1` | `qwen-plus` | bailian.console.aliyun.com |
| **豆包（Doubao）** | `https://ark.cn-beijing.volces.com/api/v3` | `doubao-1-5-pro-32k-250115` | console.volcengine.com/ark |
| **ChatGPT（OpenAI）** | `https://api.openai.com/v1` | `gpt-4o` | platform.openai.com |
| **元寶（騰訊混元）** | `https://api.hunyuan.cloud.tencent.com/v1` | `hunyuan-turbo` | console.cloud.tencent.com/hunyuan |
| **Gemini（Google）** | `https://generativelanguage.googleapis.com/v1beta/openai` | `gemini-2.0-flash` | aistudio.google.com |
| **MiniMax** | `https://api.minimax.chat/v1` | `MiniMax-Text-01` | platform.minimaxi.com |
| **Kimi（月之暗面）** | `https://api.moonshot.cn/v1` | `moonshot-v1-8k` | platform.moonshot.cn |
| **Mimo（小米）** | `https://api.mimo.ai/v1` | `mimo-1` | platform.mimo.ai |
| **Claude（Anthropic）** | `https://api.anthropic.com/v1` | `claude-sonnet-4-20250514` | console.anthropic.com |
| **Grok（xAI）** | `https://api.x.ai/v1` | `grok-2-latest` | console.x.ai |

> **注意**：模型名稱和介面位址會經常變動，請以各提供商官方文件為準。「名稱」欄位只是你取的標籤，不影響實際呼叫哪個介面。

## 資料儲存

- **設定與資料**：儲存在系統使用者資料目錄（Linux 下如 `~/.local/share/Weqi/`）。
- **AI 提供商設定**：單獨儲存在使用者資料目錄，不會寫入專案。
- **對局歷史與存檔**：儲存在使用者資料目錄。

## 授權

開源。詳見儲存庫。
