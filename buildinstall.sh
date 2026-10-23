#!/usr/bin/env bash
# 编译最新源码并安装到 ~/Applications，然后打开。用法：./buildinstall.sh
set -euo pipefail
cd "$(dirname "$0")"

./build.sh --no-open

DEST="$HOME/Applications/NotepadFast.app"
mkdir -p "$HOME/Applications"
rm -rf "$DEST"
ditto build/bin/NotepadFast.app "$DEST"

echo ">> 已安装：$DEST"
open "$DEST"
