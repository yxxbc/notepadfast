#pragma once

#include <QWidget>
#include <QHash>
#include <QUrl>
#include "ui_markdownview.h"

class QNetworkAccessManager;
class MarkdownDocument;

//Markdown 预览面板（VSCode 式侧边栏）
class MarkdownView : public QWidget
{
	Q_OBJECT

public:
	MarkdownView(QWidget *parent = nullptr);
	~MarkdownView();

	//渲染一次。baseDir 是当前 .md 所在目录，用来解析相对路径的图片。
	void setMarkdown(const QString& markdown, const QString& baseDir = QString());

	//自检用
	int documentTextLength() const;
	int loadedImageCount() const;

	void viewMarkdown(QString& text);
	void viewHtml(const QString& html);

private:
	Ui::MarkdownViewClass ui;
	MarkdownDocument* m_doc = nullptr;
	QNetworkAccessManager* m_nam = nullptr;
	QString m_lastMarkdown;   //上一次渲染的内容，内容没变就跳过
};
