#!/usr/bin/env bash
# 打包 Weqi 为 RPM（Linux，Fedora/RHEL/openSUSE 等）。
# 用法: ./package-rpm.sh
# 依赖: rpmbuild
set -euo pipefail

cd "$(dirname "$0")"

APP="weqi"
VERSION="0.1.0"
SPEC="weqi.spec"

# 检查 rpmbuild
if ! command -v rpmbuild >/dev/null 2>&1; then
    echo "错误: 未找到 rpmbuild，请先安装 rpm-build 包。" >&2
    exit 1
fi

# 临时工作目录（不污染 ~/rpmbuild）
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

mkdir -p "$WORK/BUILD" "$WORK/RPMS" "$WORK/SRPMS" "$WORK/SPECS" "$WORK/SOURCES"

# 打包源码目录（含二进制与 Python 适配器）
SRC_DIR="$WORK/SOURCES/$APP-$VERSION"
mkdir -p "$SRC_DIR/ai_adapter/providers"
cp Weqi "$SRC_DIR/Weqi"
cp ai_adapter/main.py "$SRC_DIR/ai_adapter/main.py"
cp ai_adapter/parser.py "$SRC_DIR/ai_adapter/parser.py"
cp ai_adapter/providers/__init__.py "$SRC_DIR/ai_adapter/providers/__init__.py"
cp ai_adapter/providers/openai_compatible.py "$SRC_DIR/ai_adapter/providers/openai_compatible.py"

# 生成 tar.gz 源码包
tar -C "$WORK/SOURCES" -czf "$WORK/SOURCES/$APP-$VERSION.tar.gz" "$APP-$VERSION"

# 复制 spec
cp "$SPEC" "$WORK/SPECS/$SPEC"

# 构建 RPM
rpmbuild --define "_topdir $WORK" \
         --define "_builddir $WORK/BUILD" \
         --define "_rpmdir $WORK/RPMS" \
         --define "_srcrpmdir $WORK/SRPMS" \
         --define "_specdir $WORK/SPECS" \
         --define "_sourcedir $WORK/SOURCES" \
         -bb "$WORK/SPECS/$SPEC"

# 复制生成的 RPM 到当前目录
find "$WORK/RPMS" -name "*.rpm" -exec cp {} . \;
echo "已生成:"
ls -1 *.rpm 2>/dev/null || true
