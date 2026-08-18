#!/usr/bin/env bash
# 打包 Weqi 为 Windows .exe 分发目录（需在 Windows 上运行）。
# 用法: ./package-exe.sh
# 依赖: windeployqt（随 Qt 安装）、Python 3
set -euo pipefail

cd "$(dirname "$0")"

APP="Weqi"
OUT="weqi-win"

# 清理旧的输出目录
rm -rf "$OUT"
mkdir -p "$OUT/ai_adapter/providers"

# 复制二进制与 Python 适配器
cp Weqi "$OUT/$APP.exe"
cp ai_adapter/main.py "$OUT/ai_adapter/main.py"
cp ai_adapter/parser.py "$OUT/ai_adapter/parser.py"
cp ai_adapter/providers/__init__.py "$OUT/ai_adapter/providers/__init__.py"
cp ai_adapter/providers/openai_compatible.py "$OUT/ai_adapter/providers/openai_compatible.py"

# 用 windeployqt 收集 Qt 运行库（DLL、插件、翻译等）
# 若 windeployqt 不在 PATH，请指定完整路径，例如：
#   WINDEPLOYQT="/c/Qt/6.11.1/mingw_64/bin/windeployqt.exe"
WINDEPLOYQT="${WINDEPLOYQT:-windeployqt}"
"$WINDEPLOYQT" --no-translations --no-system-d3d-compiler --no-opengl-sw \
    --dir "$OUT" "$OUT/$APP.exe"

echo "已生成分发目录: $OUT/"
echo "可直接分发，或用 Inno Setup / NSIS 进一步打成安装包。"
