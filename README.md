# NotepadFast

基于 [notepad--](https://gitee.com/cxasm/notepad--) 的跨平台文本编辑器源码，**仅维护 macOS（Apple Silicon / arm64）**。

## 目录结构

```
src/                应用源码（*.cpp / *.h / *.ui 平铺 + cceditor/），RealCompare.pro 为唯一工程文件
  Resources/        Qt 资源（qrc）：图标、样式图片  → 运行时路径 :/Resources/...
  qss/              Qt 样式表                        → :/qss/...
  notepad/          默认语言/样式模板（首次运行释放到配置目录）→ :/notepad/...
  themes/           Lexer 主题 ini，随二进制旁侧部署（非 qrc）
  macicon/          macOS 图标源文件
  plugin/           插件示例（helloworld 可作为模板）
third_party/qscint/ 内置 QScintilla 静态库（第三方，勿改）
build/              构建输出（bin/ 可执行文件、lib/ 静态库），已 gitignore
```

## 构建 / 安装

```sh
./build.sh          # 用最新源码编译并直接打开 app（首次会自动 brew install qt@5）
./buildinstall.sh   # 编译并装到 ~/Applications，然后打开
```

产物：`build/bin/NotepadFast.app`（脚本会自动把 `src/themes/` 拷进 bundle，代码按
`applicationDirPath()/themes/` 读主题）。

手动编译等价于：

```sh
cd third_party/qscint/src && qmake qscintilla.pro && make -j8   # 静态库 → build/lib
cd src && qmake RealCompare.pro && make -j8                     # 主程序 → build/bin
```

## 说明

- Markdown 预览用 [cmark-gfm](https://github.com/github/cmark-gfm) 渲染（`brew install cmark-gfm`，静态库直接链进 .app）。
- 设置页快捷键：⌘,（`QKeySequence::Preferences`）。
- 翻译：`src/realcompare_zh.ts` → `lrelease` 生成 `realcompare_zh.qm`（qrc 引用，改了文案需重新生成）。
- 插件：主程序从自身目录下的 `plugin/` 加载动态库；插件侧头文件用 `src/include/pluginGl.h`，主程序用 `src/pluginGl.h`（两者不同，别合并）。
- 第三方依赖见 [THIRDPARTY.md](THIRDPARTY.md)。
