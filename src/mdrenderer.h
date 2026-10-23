// ============================================================
// mdrenderer.h —— Markdown -> HTML 渲染器（基于 github/cmark-gfm）
//
// 语法覆盖与 GitHub 网页端一致：表格、删除线、自动链接、任务列表、
// 围栏代码块、嵌套列表、脚注以外的常见 GFM 特性。
// 第三方：cmark-gfm（BSD-2-Clause），brew install cmark-gfm。
// ============================================================

#pragma once

#include <QString>
#include <QStringList>

namespace MdRenderer {

// 把 Markdown 文本渲染为 HTML 片段（不含 <html><head>）。
QString renderToHtml(const QString& markdown);

} // namespace MdRenderer