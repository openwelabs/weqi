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

## 使用说明

- **选中棋子**：左键点击任意棋子，该格会高亮。
- **移动棋子**：选中后点击目标格，本地引擎会校验走法合法性。
- **悔棋**：使用「悔棋」按钮撤销一步。
- **新游戏**：在对局页或首页开始新对局。
- **AI 对战**：选择两个 AI 提供商，使用「开始 / 暂停 / 继续 / 停止」控制自动对局。
- **切换语言**：打开「设置 → 语言」，选择 7 种支持的语言之一（或「跟随系统」）。

## 数据存储

- **设置与资料**：存储在系统用户数据目录（Linux 下如 `~/.local/share/Weqi/`）。
- **AI 提供商配置**：单独存储在用户数据目录，不会写入项目。
- **对局历史与存档**：存储在用户数据目录。

## 许可证

开源。详见仓库。
