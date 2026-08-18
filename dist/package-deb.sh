#!/usr/bin/env bash
# 打包 Weqi 为 .deb（Linux）。
# 用法: ./package-deb.sh
# 依赖: dpkg-deb, fakeroot（可选）
set -euo pipefail

cd "$(dirname "$0")"

APP="weqi"
VERSION="0.1.0"
ARCH="amd64"
PKG="${APP}_${VERSION}_${ARCH}.deb"

# 临时打包目录
STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT

# 目录结构
mkdir -p "$STAGE/DEBIAN"
mkdir -p "$STAGE/usr/bin"
mkdir -p "$STAGE/usr/share/$APP/ai_adapter/providers"

# 控制文件
cat > "$STAGE/DEBIAN/control" <<EOF
Package: $APP
Version: $VERSION
Section: games
Priority: optional
Architecture: $ARCH
Depends: libqt6widgets6 (>= 6.2), libqt6gui6, libqt6core6, python3
Maintainer: Weqi Developers <liuhuiquan12022@outlook.com>
Description: A modern open-source desktop chess application
 Weqi is a modern, clean open-source desktop chess application.
 All chess rules are implemented in a local C++ engine, with support
 for human vs human, human vs AI, AI vs AI, and game replay.
Homepage: https://github.com/openwelabs/weqi
EOF

# 安装文件
install -m 0755 Weqi "$STAGE/usr/bin/$APP"
install -m 0644 ai_adapter/main.py "$STAGE/usr/share/$APP/ai_adapter/main.py"
install -m 0644 ai_adapter/parser.py "$STAGE/usr/share/$APP/ai_adapter/parser.py"
install -m 0644 ai_adapter/providers/__init__.py "$STAGE/usr/share/$APP/ai_adapter/providers/__init__.py"
install -m 0644 ai_adapter/providers/openai_compatible.py "$STAGE/usr/share/$APP/ai_adapter/providers/openai_compatible.py"

# 打包
dpkg-deb --build --root-owner-group "$STAGE" "$PKG"
echo "已生成: $PKG"
