// ============================================================
// mdrenderer.cpp —— Markdown -> HTML（基于 github/cmark-gfm）
//
// 早期是自研解析器（约 1300 行），覆盖不了常见 GFM 边界情况；
// 现在直接调用 GitHub 官方 C 实现 cmark-gfm（BSD-2-Clause），
// 语法覆盖与 GitHub 网页端一致，代码量降到几十行。
//
// 依赖：brew install cmark-gfm（静态库随 .app 一起链进去）
// ============================================================

#include "mdrenderer.h"

#include <QRegularExpression>

#include <cmark-gfm.h>
#include <cmark-gfm-core-extensions.h>

namespace {

// 只注册一次 core extensions
void ensureExtensionsRegistered()
{
	static const bool done = []() {
		cmark_gfm_core_extensions_ensure_registered();
		return true;
	}();
	Q_UNUSED(done);
}

// QTextDocument 不支持 <input>，把 GFM 任务列表的复选框换成字符
QString fixTaskList(const QString& html)
{
	static const QRegularExpression checked(QStringLiteral("<input[^>]*checked[^>]*>"));
	static const QRegularExpression unchecked(QStringLiteral("<input[^>]*>"));
	QString out = html;
	out.replace(checked, QStringLiteral("☑"));
	out.replace(unchecked, QStringLiteral("☐"));
	return out;
}

} // namespace

namespace MdRenderer {

QString renderToHtml(const QString& markdown)
{
	ensureExtensionsRegistered();

	const int options = CMARK_OPT_DEFAULT | CMARK_OPT_UNSAFE; // 本地预览，允许内嵌 HTML
	cmark_parser* parser = cmark_parser_new(options);

	static const char* kExtensions[] = { "table", "strikethrough", "autolink", "tasklist" };
	for (const char* name : kExtensions)
	{
		if (cmark_syntax_extension* ext = cmark_find_syntax_extension(name))
		{
			cmark_parser_attach_syntax_extension(parser, ext);
		}
	}

	const QByteArray input = markdown.toUtf8();
	cmark_parser_feed(parser, input.constData(), static_cast<size_t>(input.size()));

	cmark_node* doc = cmark_parser_finish(parser);
	if (doc == nullptr)
	{
		cmark_parser_free(parser);
		return QString();
	}

	char* raw = cmark_render_html(doc, options, cmark_parser_get_syntax_extensions(parser));
	const QString html = raw != nullptr ? fixTaskList(QString::fromUtf8(raw)) : QString();

	if (raw != nullptr)
	{
		free(raw);
	}
	cmark_node_free(doc);
	cmark_parser_free(parser);
	return html;
}

} // namespace MdRenderer
