#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QSvgRenderer>
#include <QDir>
#include <QDebug>

int main(int argc, char** argv)
{
	QGuiApplication app(argc, argv);
	if (argc < 4) { qWarning() << "usage: svg2png in.svg out.png size"; return 1; }
	const QString in = argv[1], out = argv[2];
	const int size = QString(argv[3]).toInt();
	QSvgRenderer r(in);
	if (!r.isValid()) { qWarning() << "invalid svg" << in; return 2; }
	QImage img(size, size, QImage::Format_ARGB32_Premultiplied);
	img.fill(Qt::transparent);
	QPainter p(&img);
	p.setRenderHint(QPainter::Antialiasing, true);
	r.render(&p);
	p.end();
	if (!img.save(out)) { qWarning() << "save failed"; return 3; }
	qInfo() << "saved" << out << size;
	return 0;
}
