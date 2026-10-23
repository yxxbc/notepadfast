#pragma once

#include <QWidget>
#include "ui_markdownview.h"

class MarkdownView : public QWidget
{
	Q_OBJECT

public:
	MarkdownView(QWidget *parent = nullptr);

	//编辑区内容变化时直接刷新（侧边面板模式）
	void setMarkdown(const QString& markdown);

	//自检用：当前渲染出的纯文本长度
	int documentTextLength() const;
	~MarkdownView();
	void viewMarkdown(QString& text);

	// 以 HTML 呈现。
	//
	// 为什么需要这个入口：
	//   viewMarkdown() 走 QTextDocument::setMarkdown()，
	//   渲染能力受 Qt 内置实现限制（GFM 表格、任务列表支持有限）。
	//   本工程自带自研渲染器 MdRenderer（src/mdrenderer.*），
	//   覆盖 GFM 表格 / 任务列表 / 嵌套列表等，先转成 HTML
	//   再交给这里显示，效果更好。
	void viewHtml(const QString& html);

private:
	Ui::MarkdownViewClass ui;
};
