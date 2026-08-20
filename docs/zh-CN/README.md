# Weqi

**Weqi** 是一款现代、简洁的开源桌面国际象棋应用。所有棋规均在本地 C++ 引擎中实现，支持真人对战、人机对战、AI 对战与复盘。

> **选择语言阅读本文档：**
> [English](../../README.md) · [繁體中文](../zh-TW/README.md) · [日本語](../ja/README.md) · [Español](../es/README.md) · [Українська](../uk/README.md) · [한국어](../ko/README.md)

---

## 功能特性

- **四种游戏模式**
  - **真人对战** — 同一设备上双人对弈。
  - **人机对战** — 与所选 AI 提供商对弈。
  - **AI 对战** — 两个 AI 自动对弈（不计入你的战绩）。
  - **复盘** — 回顾历史对局。
- **完整棋规** 在本地 C++ 中实现（合法走法校验、将军、将死、逼和、王车易位、吃过路兵、升变等）。
- **玩家资料与 Rating** — 追踪你的 Rating、最佳 Rating，并可编辑玩家名称。
- **战绩统计** — 对局数、胜率、胜、和、负、最高连胜、当前连胜、最佳 Rating。
- **对局历史** — 浏览过往对局，包含日期、模式、对手、结果与 Rating 变化。
- **继续未完成对局** — 随时恢复进行中的对局。
- **AI Providers** — 添加、编辑、删除 AI 提供商（名称、类型、Base URL、API Key、模型）。API Key 为私密数据，仅保存在系统用户数据目录，不会写入项目。
- **AI 聊天** — AI 会对其走法做简短点评，消息语言跟随界面语言。
- **国际化（i18n）** — 7 种界面语言：简体中文、繁体中文、英语、日语、西班牙语、乌克兰语、韩语。运行时切换语言，无需重启。

## 技术栈

- **C++17**
- **Qt 6**（Widgets）
- **CMake**

## 目录结构

```
weqi/
├── CMakeLists.txt
├── README.md
├── ai_adapter/          # Python AI 适配器（语言检测、重试/调教）
│   ├── main.py
│   ├── parser.py
│   └── providers/
│       └── openai_compatible.py
├── dist/                # 可分发目录（编译好的二进制 + 打包脚本）
├── resources/           # Qt 资源文件
├── scripts/             # 辅助脚本
├── src/
│   ├── main.cpp         # 程序入口
│   ├── MainWindow.h/.cpp
│   ├── GameController.h/.cpp
│   ├── ChessBoard.h/.cpp
│   ├── ChessPiece.h/.cpp
│   ├── ai/              # AI 管理器
│   ├── data/            # 设置、语言、资料、战绩、历史管理器
│   └── pages/           # 首页、新对局、对局、人机、AI 对战、历史、设置、关于
├── tests/               # C++ 测试
├── tools/               # 开发工具
└── translations/        # Qt .ts 翻译文件（7 种语言）
```

## 构建

### 依赖

- CMake ≥ 3.16
- Qt 6（≥ 6.2，含 Widgets 模块）
- 支持 C++17 的编译器（GCC / Clang）
- Python 3（用于 AI 适配器）

### 构建步骤

```bash
# 1. 配置
cmake -S . -B build

# 2. 编译
cmake --build build -j

# 3. 运行
./build/Weqi
```

也可以使用一行命令：

```bash
cmake -S . -B build && cmake --build build -j && ./build/Weqi
```

### 翻译

翻译文件位于 `translations/`，通过 Qt 的 `qt_add_translations` 编译进二进制。修改 `tr()` 字符串后更新翻译：

```bash
# 更新 .ts 文件（提取新字符串）
lupdate src -ts translations/weqi_<lang>.ts

# 发布 .qm 文件
lrelease translations/weqi_<lang>.ts
```

## 打包

`dist/` 目录包含编译好的二进制与 Python AI 适配器，可直接打包成安装包。

```
dist/
├── Weqi                  # 编译好的 C++ 二进制（Qt6 Widgets，内嵌 7 种语言）
├── ai_adapter/           # Python AI 适配器
├── package-deb.sh        # 打包 .deb（Linux）
├── package-rpm.sh        # 打包 .rpm（Linux，Fedora/RHEL/openSUSE）
├── weqi.spec             # RPM spec 文件
├── package-exe.sh        # 打包 .exe（Windows，需在 Windows 上运行）
├── package-appimage.sh   # 打包 .AppImage（Linux，任意发行版）
├── weqi.desktop          # AppImage 桌面入口
├── weqi.png              # AppImage 图标（512×512）
└── README.md
```

- **.deb**：`cd dist && ./package-deb.sh` → `weqi_0.1.0_amd64.deb`
- **.rpm**：`cd dist && ./package-rpm.sh` → `weqi-0.1.0-1.fc44.x86_64.rpm`
- **.exe**：在 Windows 上运行 `./package-exe.sh` → `weqi-win/` 目录（使用 `windeployqt`）
- **.AppImage**：`cd dist && ./package-appimage.sh` → `Weqi-0.1.0-x86_64.AppImage`（单文件，无需安装）

二进制会相对于可执行文件查找 `ai_adapter/main.py`；通过安装包安装时，则从 `/usr/share/weqi/ai_adapter/main.py` 加载。

## 使用说明

- **选中棋子**：左键点击任意棋子，该格会高亮。
- **移动棋子**：选中后点击目标格，本地引擎会校验走法合法性。
- **悔棋**：使用「悔棋」按钮撤销一步。
- **新游戏**：在对局页或首页开始新对局。
- **AI 对战**：选择两个 AI 提供商，使用「开始 / 暂停 / 继续 / 停止」控制自动对局。
- **切换语言**：打开「设置 → 语言」，选择 7 种支持的语言之一（或「跟随系统」）。

## 添加 AI 模型

要跟 AI 对弈，需要先在「设置 → AI Providers」中添加一个 AI 提供商。每个提供商有 5 个字段：

| 字段 | 填写内容 |
| --- | --- |
| **名称** | 你自己起的显示名称，例如 `DeepSeek`、`Qwen`、`豆包`。 |
| **提供商类型** | 下面这些模型都使用 OpenAI 兼容接口，统一填 `OpenAI Compatible`。 |
| **Base URL** | 提供商的 API 地址（见下表）。 |
| **API Key** | 你在提供商控制台申请的私密密钥，只保存在本地用户数据目录，不会写入项目。 |
| **模型** | 精确的模型名称（见下表）。 |

### 常见模型推荐配置

下面这些提供商都提供 OpenAI 兼容接口，所以「提供商类型」全部填 `OpenAI Compatible`。

| 提供商 | Base URL | 模型（示例） | 申请 API Key 的地方 |
| --- | --- | --- | --- |
| **DeepSeek** | `https://api.deepseek.com/v1` | `deepseek-chat` | platform.deepseek.com |
| **Qwen（通义千问）** | `https://dashscope.aliyuncs.com/compatible-mode/v1` | `qwen-plus` | bailian.console.aliyun.com |
| **豆包（Doubao）** | `https://ark.cn-beijing.volces.com/api/v3` | `doubao-1-5-pro-32k-250115` | console.volcengine.com/ark |
| **ChatGPT（OpenAI）** | `https://api.openai.com/v1` | `gpt-4o` | platform.openai.com |
| **元宝（腾讯混元）** | `https://api.hunyuan.cloud.tencent.com/v1` | `hunyuan-turbo` | console.cloud.tencent.com/hunyuan |
| **Gemini（谷歌）** | `https://generativelanguage.googleapis.com/v1beta/openai` | `gemini-2.0-flash` | aistudio.google.com |
| **MiniMax** | `https://api.minimax.chat/v1` | `MiniMax-Text-01` | platform.minimaxi.com |
| **Kimi（月之暗面）** | `https://api.moonshot.cn/v1` | `moonshot-v1-8k` | platform.moonshot.cn |
| **Mimo（小米）** | `https://api.mimo.ai/v1` | `mimo-1` | platform.mimo.ai |

> **注意**：模型名称和接口地址会经常变动，请以各提供商官方文档为准。「名称」字段只是你起的标签，不影响实际调用哪个接口。

## 数据存储

- **设置与资料**：存储在系统用户数据目录（Linux 下如 `~/.local/share/Weqi/`）。
- **AI 提供商配置**：单独存储在用户数据目录，不会写入项目。
- **对局历史与存档**：存储在用户数据目录。

## 许可证

开源。详见仓库。
