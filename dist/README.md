# Weqi 可分发目录

本目录包含编译好的 **Weqi** 可执行文件与 Python AI 适配器，用于打包成
`.deb`（Linux）、`.exe`（Windows）等安装包。

## 目录结构

```
dist/
├── Weqi                  # 编译好的 C++ 二进制（Qt6 Widgets，含 7 种语言 .qm）
├── ai_adapter/           # Python AI 适配器（C++ 通过 QProcess 调用）
│   ├── main.py
│   ├── parser.py
│   ├── test_i18n_ai.py
│   ├── test_parser.py
│   └── providers/
│       ├── __init__.py
│       └── openai_compatible.py
├── package-deb.sh        # 打包 .deb（Linux）
├── package-rpm.sh        # 打包 .rpm（Linux，Fedora/RHEL/openSUSE）
├── weqi.spec             # RPM spec 文件
├── package-exe.sh        # 打包 .exe（Windows，需在 Windows 上运行）
└── README.md
```

## 运行依赖

- **Linux**: Qt 6（≥ 6.2，Widgets 模块）、Python 3
- **Windows**: Qt 6（≥ 6.2，Widgets 模块）、Python 3

> 二进制链接的是系统 Qt6 共享库，未静态打包。打包安装包时需一并带上
> Qt 运行库（见下方各平台说明）。

## 二进制如何找到 AI 适配器

`Weqi` 启动时会按以下顺序查找 `ai_adapter/main.py`：

1. `<可执行文件目录>/ai_adapter/main.py`
2. `<可执行文件目录>/../ai_adapter/main.py`
3. `<可执行文件目录>/../../ai_adapter/main.py`
4. `/usr/share/weqi/ai_adapter/main.py`（Linux 打包安装时）

因此本目录中 `Weqi` 与 `ai_adapter/` 平级即可正常工作；RPM/deb 安装到
`/usr/bin/weqi` 时，适配器位于 `/usr/share/weqi/ai_adapter/` 也能被找到。

## 打包 .deb（Linux）

```bash
./package-deb.sh
```

脚本会生成 `weqi_0.1.0_amd64.deb`，安装到 `/usr/bin/weqi` 与
`/usr/share/weqi/`。安装后运行 `weqi` 即可。

## 打包 .rpm（Linux）

```bash
./package-rpm.sh
```

脚本会生成 `weqi-0.1.0-1.fc44.x86_64.rpm`（需已安装 `rpm-build`）。
安装到 `/usr/bin/weqi` 与 `/usr/share/weqi/`。安装后运行 `weqi` 即可。

## 打包 .exe（Windows）

在 Windows 上执行：

```bash
./package-exe.sh
```

脚本使用 `windeployqt` 收集 Qt 运行库，并生成 `Weqi.exe` 与
`ai_adapter/` 到 `weqi-win/` 目录，可直接分发或进一步用
Inno Setup / NSIS 打成安装包。

## 数据存储

程序数据（设置、配置、战绩、历史、存档）保存在系统用户数据目录：

- Linux: `~/.local/share/Weqi/`
- Windows: `%APPDATA%/Weqi/`

AI 提供商的 API Key 仅保存在该目录的 `config/ai_providers.json`，不会写入项目。
