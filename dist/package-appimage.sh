#!/usr/bin/env bash
# 打包 Weqi 为 AppImage（Linux，任意发行版）。
# 用法: ./package-appimage.sh
# 依赖: curl, qmake6（或 qmake），首次运行会自动下载 linuxdeploy 工具。
set -euo pipefail

cd "$(dirname "$0")"

APP="Weqi"
VERSION="0.1.0"
OUTPUT="${APP}-${VERSION}-x86_64.AppImage"

# 定位 qmake（linuxdeploy-plugin-qt 需要）
QMAKE_BIN="$(command -v qmake6 || command -v qmake || true)"
if [ -z "$QMAKE_BIN" ]; then
    echo "错误: 未找到 qmake6/qmake，请安装 Qt6 开发包。" >&2
    exit 1
fi
export QMAKE="$QMAKE_BIN"

# 工具目录（缓存到 ~/.cache/weqi-appimage）
TOOL_DIR="${XDG_CACHE_HOME:-$HOME/.cache}/weqi-appimage"
mkdir -p "$TOOL_DIR"
LINUXDEPLOY="$TOOL_DIR/linuxdeploy-x86_64.AppImage"
QT_PLUGIN="$TOOL_DIR/linuxdeploy-plugin-qt-x86_64.AppImage"

download() {
    local url="$1" out="$2"
    if [ ! -x "$out" ]; then
        echo "下载 $out ..."
        curl -sL -o "$out" "$url"
        chmod +x "$out"
    fi
}

download "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" "$LINUXDEPLOY"
download "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage" "$QT_PLUGIN"

# 临时 AppDir
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
APPDIR="$WORK/AppDir"

# 组装 AppDir 结构
mkdir -p "$APPDIR/usr/bin/ai_adapter/providers"
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor/512x512/apps"

cp Weqi "$APPDIR/usr/bin/Weqi"
cp ai_adapter/main.py "$APPDIR/usr/bin/ai_adapter/main.py"
cp ai_adapter/parser.py "$APPDIR/usr/bin/ai_adapter/parser.py"
cp ai_adapter/providers/__init__.py "$APPDIR/usr/bin/ai_adapter/providers/__init__.py"
cp ai_adapter/providers/openai_compatible.py "$APPDIR/usr/bin/ai_adapter/providers/openai_compatible.py"
cp weqi.desktop "$APPDIR/usr/share/applications/weqi.desktop"
cp weqi.png "$APPDIR/usr/share/icons/hicolor/512x512/apps/weqi.png"

# 额外收集 Wayland 平台插件（linuxdeploy-plugin-qt 默认只收集 xcb）
# 在 Wayland 会话（如 niri）下 Qt 优先使用 wayland 插件，缺失会导致启动失败
QT_PLUGIN_DIR="$("$QMAKE_BIN" -query QT_INSTALL_PLUGINS 2>/dev/null || true)"
if [ -n "$QT_PLUGIN_DIR" ] && [ -f "$QT_PLUGIN_DIR/platforms/libqwayland.so" ]; then
    mkdir -p "$APPDIR/usr/plugins/platforms"
    cp "$QT_PLUGIN_DIR/platforms/libqwayland.so" "$APPDIR/usr/plugins/platforms/"
    # wayland 平台插件还需要 shell integration（xdg-shell 等）
    if [ -d "$QT_PLUGIN_DIR/wayland-shell-integration" ]; then
        cp -r "$QT_PLUGIN_DIR/wayland-shell-integration" "$APPDIR/usr/plugins/"
    fi
    echo "已加入 Wayland 平台插件与 shell integration"
fi

# 运行 linuxdeploy：收集 Qt 依赖（只处理 AppDir，不打包）
# NO_STRIP=1：跳过 strip（新版 Qt 库含 .relr.dyn section，linuxdeploy 的 strip 无法处理）
export NO_STRIP=1
export LINUXDEPLOY_PLUGIN_QT_IGNORE_UNSUPPORTED=1
"$LINUXDEPLOY" \
    --appdir "$APPDIR" \
    --executable "$APPDIR/usr/bin/Weqi" \
    --desktop-file "$APPDIR/usr/share/applications/weqi.desktop" \
    --icon-file "$APPDIR/usr/share/icons/hicolor/512x512/apps/weqi.png" \
    --plugin qt

# 修复 linuxdeploy 破坏的 ELF 文件。
# linuxdeploy 用 patchelf 给库加 RUNPATH=$ORIGIN 时，会把 .init 段从 0x2cc 移到别处，
# 但不会更新动态段里的 DT_INIT 标签，导致动态链接器跳转到零填充内存 → SIGSEGV。
# 修复方式：用系统原始版本替换所有 DT_INIT 与 .init 段地址不一致的库/插件。
fix_corrupted_elf() {
    local dir="$1" sysroot="$2"
    local fixed=0
    while IFS= read -r f; do
        # 只处理 ELF 且带 DT_INIT 的动态库/插件
        if ! file "$f" 2>/dev/null | grep -q "ELF"; then
            continue
        fi
        local init initsec
        init="$(readelf -d "$f" 2>/dev/null | awk '/\(INIT\)/ {print $NF}')"
        [ -n "$init" ] || continue
        initsec="$(readelf -S "$f" 2>/dev/null | awk '/\.init / {print $4}')"
        # .init 段地址是 16 位十六进制（无 0x 前缀），如 0000000000052f98
        if ! echo "$initsec" | grep -qE '^[0-9a-f]{16}$'; then
            continue
        fi
        if [ "$init" = "0x$initsec" ]; then
            continue
        fi
        # 在系统目录里找同名文件替换
        local rel sys
        rel="${f#"$dir"/}"
        sys="$sysroot/$rel"
        if [ -f "$sys" ]; then
            cp "$sys" "$f"
            echo "  修复: $rel"
            fixed=$((fixed + 1))
        else
            echo "  警告: 系统无对应文件，跳过 $rel" >&2
        fi
    done < <(find "$dir" -type f \( -name '*.so*' -o -name 'Weqi' \) 2>/dev/null)
    echo "已修复 $fixed 个被破坏的 ELF 文件"
}

# 用系统 Qt 库/插件替换被破坏的文件。
# 库在 $(qmake -query QT_INSTALL_LIBS)，插件在 $(qmake -query QT_INSTALL_PLUGINS)。
QT_LIB_DIR="$("$QMAKE_BIN" -query QT_INSTALL_LIBS 2>/dev/null || echo /usr/lib64)"
QT_PLUGIN_DIR="$("$QMAKE_BIN" -query QT_INSTALL_PLUGINS 2>/dev/null || echo /usr/lib64/qt6/plugins)"
echo "修复 AppDir 中被 linuxdeploy 破坏的 ELF 文件..."
fix_corrupted_elf "$APPDIR/usr/lib" "$QT_LIB_DIR"
fix_corrupted_elf "$APPDIR/usr/plugins" "$QT_PLUGIN_DIR"

# 用 linuxdeploy-plugin-appimage 打包
APPIMAGE_PLUGIN="$TOOL_DIR/linuxdeploy-plugin-appimage-x86_64.AppImage"
download "https://github.com/linuxdeploy/linuxdeploy-plugin-appimage/releases/download/continuous/linuxdeploy-plugin-appimage-x86_64.AppImage" "$APPIMAGE_PLUGIN"
"$APPIMAGE_PLUGIN" --appdir "$APPDIR"

# linuxdeploy 在当前目录生成 <Name>-x86_64.AppImage，重命名为带版本号
if [ -f "${APP}-x86_64.AppImage" ]; then
    mv -f "${APP}-x86_64.AppImage" "$OUTPUT"
fi
echo "已生成: $OUTPUT"
