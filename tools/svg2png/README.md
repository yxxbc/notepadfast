# svg2png

把 SVG 图标渲染成带透明背景的 PNG（图标进 qrc 前跑一次）。

不直接用 `qlmanage`：它会把 24×24 的 SVG 画在 192×192 画布左上角，缩放后图标只剩一个小点。

```sh
cd tools/svg2png && qmake svg2png.pro && make -j4
QT_QPA_PLATFORM=offscreen ./svg2png.app/Contents/MacOS/svg2png ../../src/Resources/icons/markdown.svg ../../src/Resources/icons/markdown.png 96
```
