#!/usr/bin/env bash
# 用最新源码编译并打开 app。用法：./build.sh [--no-open]
set -euo pipefail
cd "$(dirname "$0")"

if ! QT_PREFIX="$(brew --prefix qt@5 2>/dev/null)" || [ ! -d "$QT_PREFIX" ]; then
    echo ">> 首次运行，安装 Qt5（brew install qt@5）..."
    brew install qt@5
    QT_PREFIX="$(brew --prefix qt@5)"
fi
export PATH="$QT_PREFIX/bin:$PATH"

JOBS="$(sysctl -n hw.ncpu)"
ROOT="$PWD"

# 1) QScintilla 静态库 → build/lib（增量编译）
echo ">> [1/3] QScintilla"
cd "$ROOT/third_party/qscint/src"
[ -f Makefile ] || qmake qscintilla.pro
make -j"$JOBS"

# 2) 主程序 → build/bin/NotepadFast.app
echo ">> [2/3] NotepadFast"
cd "$ROOT/src"
[ -f Makefile ] || qmake RealCompare.pro
make -j"$JOBS"

# 3) 资源：代码按 applicationDirPath()/themes 读主题 ini，拷进 bundle
echo ">> [3/3] 拷资源"
APP="$ROOT/build/bin/NotepadFast.app"
rm -rf "$APP/Contents/MacOS/themes"
cp -R "$ROOT/src/themes" "$APP/Contents/MacOS/themes"

echo ">> 完成：$APP"
[ "${1:-}" = "--no-open" ] || open "$APP"
