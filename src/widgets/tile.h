// Tile — 磁贴卡片组件 (对应原 Tile.tsx)
#ifndef TILE_H
#define TILE_H

#include <QWidget>
#include <QString>
#include <QColor>
#include <QScrollArea>
#include <QLabel>

class Tile : public QWidget {
	Q_OBJECT
public:
	explicit Tile(QWidget *parent = nullptr);

	void setTileTitle(const QString &title);
	void setTileContent(const QString &content);
	void setTileColor(const QColor &color);
	void setTileFontSize(int fontSize);
	void setMarkdownTheme(const QString &theme) { m_markdownTheme = theme; }
	void refreshForThemeChange();

	QString tileTitle() const { return m_title; }
	QString tileContent() const { return m_content; }

signals:
	void clicked();
	void contextMenuRequested(const QPoint &pos);

protected:
	bool eventFilter(QObject *watched, QEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;

private:
	void updateDerivedColors();

	QString m_title;
	QString m_content;
	QColor m_bgColor = QColor("#f6f3ec");
	QColor m_borderColor;
	QColor m_cornerColor;
	QColor m_titleColor;
	int m_fontSize = 14;
	bool m_hovered = false;
	QPointF m_dragStartPos;
	QScrollArea *m_scroll = nullptr;
	QWidget *m_viewport = nullptr;
	QWidget *m_contentWidget = nullptr;
	QLabel *m_titleLabel = nullptr;
	QLabel *m_contentLabel = nullptr;
	QString m_markdownTheme = "github";
};

#endif // TILE_H
