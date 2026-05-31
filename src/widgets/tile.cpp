#include "tile.h"
#include "utils/colorutils.h"
#include "app/theme.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QDebug>
#include <QEvent>

extern "C" {
#include "3rdparty/md4c/md4c-html.h"
}

static QString md4cToHtml(const QString &markdown,
                           const QString &borderColor,
                           const QString &altBg,
                           const QString &textSoft,
                           const QString &codeTxt,
                           const QString &preBg,
                           const QString &preTxt,
                           const QString &link,
                           const QString &quoteBorder,
                           const QString &quoteText,
                           const QString &h1Line,
                           const QString &hrBg) {
    QByteArray input = markdown.toUtf8();
    QByteArray output;
    md_html(input.constData(), input.size(),
            [](const MD_CHAR *html, MD_SIZE size, void *userdata) {
                static_cast<QByteArray *>(userdata)->append(html, size);
            }, &output,
            MD_FLAG_TABLES | MD_FLAG_STRIKETHROUGH | MD_FLAG_TASKLISTS,
            0);
    QString html = QString::fromUtf8(output);

    html = "<div>" + html + "</div>";
    int divStart = html.indexOf("<div style=\"border:");
    while (divStart != -1) {
        int divEnd = html.indexOf("</div>", divStart);
        if (divEnd == -1) break;
        divEnd += 6;
        int tStart = html.indexOf("<table>", divStart);
        int tEnd = html.lastIndexOf("</table>");
        if (tStart != -1 && tEnd > tStart && tStart >= divStart && tEnd < divEnd) {
            QString tablePart = html.mid(tStart, tEnd - tStart + 8);
            html = html.mid(0, divStart) + tablePart + html.mid(divEnd);
        } else {
            html = html.mid(0, divStart) + html.mid(divEnd);
        }
        divStart = html.indexOf("<div style=\"border:", divStart);
    }

    html.replace("<table>", "<table border=\"0\" cellspacing=\"0\" cellpadding=\"0\">");
    html.replace("<th>", QString(
        "<th style=\"margin:0;border:1px solid %1;padding:6px 13px;text-align:left;background:%2;\">").arg(borderColor).arg(altBg));
    html.replace("<td>", QString(
        "<td style=\"margin:0;border:1px solid %1;padding:6px 13px;text-align:left;\">").arg(borderColor));
    html.replace("<thead>", "").replace("</thead>", "");
    html.replace("<tbody>", "").replace("</tbody>", "");

    QString css = QString(R"(
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: "Noto Sans SC", "Source Han Sans SC", system-ui, sans-serif;
            font-size: %1px;
            color: %2;
            line-height: 1.7;
            background: transparent;
        }
        h1, h2, h3, h4 {
            font-family: "Noto Serif SC", "Source Han Serif SC", Georgia, serif;
            font-weight: bold;
            line-height: 1.3;
            margin: 0.5em 0 0.25em 0;
        }
        h1 { font-size: 1.15em; border-bottom: 1px solid %3; padding-bottom: 0.2em; }
        h2 { font-size: 1.05em; }
        h3 { font-size: 1em; }
        p { margin: 0.3em 0; }
        strong { font-weight: 600; }
        em { font-style: italic; }
        a { color: %4; text-decoration: underline; }
        hr { border: none; height: 3px; background: %5; margin: 8px 0; }
        blockquote {
            border-left: 3px solid %6;
            padding: 0 10px;
            color: %7;
        }
        blockquote p { padding: 0 1rem 0 0.5rem; }
        ul, ol { padding-left: 24px; margin: 0.25em 0; }
        li { margin: 2px 0; }
        pre {
            background: %8;
            color: %9;
            border: 1px solid %3;
            border-radius: 3px;
            padding: 0.5em 1em;
            overflow-x: auto;
            font-size: 0.875em;
            font-family: "JetBrains Mono", "Fira Code", ui-monospace, monospace;
        }
        pre code { background: transparent; color: inherit; padding: 0; font-size: inherit; }
        code {
            border: 1px solid %3;
            background: %8;
            color: %10;
            border-radius: 3px;
            padding: 1px 4px;
            font-size: 0.9em;
            font-family: "JetBrains Mono", "Fira Code", ui-monospace, monospace;
        }
        table { width: 100%%; font-size: 0.875em; }
        del { text-decoration: line-through; color: %2; }
        input[type="checkbox"] { accent-color: #4183C4; margin-right: 4px; }
    )").arg(13).arg(textSoft).arg(h1Line).arg(link)
       .arg(hrBg).arg(quoteBorder).arg(quoteText)
       .arg(preBg).arg(preTxt).arg(codeTxt);

    return QString("<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
                  "<style>" + css + "</style></head>"
                  "<body>" + html + "</body></html>");
}

// ─── Tile ─────────────────────────────────────────────────────

Tile::Tile(QWidget *parent)
    : QWidget(parent)
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	setMinimumSize(200, 120);
	setCursor(Qt::PointingHandCursor);
	setMouseTracking(true);

	m_scroll = new QScrollArea(this);
	m_scroll->setFrameShape(QFrame::NoFrame);
	m_scroll->setWidgetResizable(true);
	m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	m_scroll->setMouseTracking(true);
	m_scroll->setStyleSheet(
		"QScrollBar:vertical { width: 6px; background: transparent; } "
		"QScrollBar::handle:vertical { background: #b8b8ae; border-radius: 3px; min-height: 30px; } "
		"QScrollBar::handle:vertical:hover { background: #8a8a80; } "
		"QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; border: none; } "
		"QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }"
	);
	m_scroll->viewport()->setAutoFillBackground(false);

	m_contentWidget = new QWidget;
	auto *contentLayout = new QVBoxLayout(m_contentWidget);
	contentLayout->setContentsMargins(16, 16, 16, 16);
	contentLayout->setSpacing(6);
	m_titleLabel = new QLabel;
	m_titleLabel->setWordWrap(true);
	m_titleLabel->setTextFormat(Qt::PlainText);
	m_titleLabel->setStyleSheet("background: transparent; border: none;");
	contentLayout->addWidget(m_titleLabel);
	m_contentLabel = new QLabel;
	m_contentLabel->setWordWrap(true);
	m_contentLabel->setTextFormat(Qt::RichText);
	m_contentLabel->setOpenExternalLinks(false);
	m_contentLabel->setStyleSheet("background: transparent; border: none;");
	m_contentLabel->setAutoFillBackground(false);
	contentLayout->addWidget(m_contentLabel, 1);
	contentLayout->addStretch();

	m_scroll->setWidget(m_contentWidget);
	m_viewport = m_scroll->viewport();
	m_viewport->setAutoFillBackground(false);

	auto *vbox = new QVBoxLayout(this);
	vbox->setContentsMargins(0, 0, 0, 0);
	vbox->setSpacing(0);
	vbox->addWidget(m_scroll, 1);

	m_viewport->installEventFilter(this);
	updateDerivedColors();
}

bool Tile::eventFilter(QObject *watched, QEvent *event) {
	if (watched != m_viewport) return QWidget::eventFilter(watched, event);

	switch (event->type()) {
	case QEvent::MouseButtonPress:
		if (static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
			m_dragStartPos = static_cast<QMouseEvent*>(event)->globalPosition();
		}
		break;
	case QEvent::MouseButtonRelease: {
		auto *me = static_cast<QMouseEvent*>(event);
		if (me->button() == Qt::LeftButton) {
			QPointF diff = me->globalPosition() - m_dragStartPos;
			if (diff.manhattanLength() < 3) {
				emit clicked();
			}
		}
		break;
	}
	case QEvent::ContextMenu:
		emit contextMenuRequested(static_cast<QContextMenuEvent*>(event)->globalPos());
		return true;
	case QEvent::Enter:
		m_hovered = true;
		update();
		break;
	case QEvent::Leave:
		m_hovered = false;
		update();
		break;
	default:
		break;
	}
	return QWidget::eventFilter(watched, event);
}

void Tile::paintEvent(QPaintEvent *event) {
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);

	QRectF r = rect().adjusted(0.5, 0.5, -0.5, -0.5);
	double radius = 12.0;

	p.fillRect(rect(), m_bgColor);

	QPainterPath path;
	path.addRoundedRect(r, radius, radius);
	p.fillPath(path, m_bgColor);

	p.setPen(QPen(m_borderColor, 1));
	p.drawRoundedRect(r, radius, radius);

	const int markSize = 8;
	const int markOffset = 6;
	p.setPen(QPen(m_cornerColor, 0.8));
	p.setBrush(Qt::NoBrush);

	auto drawCornerMark = [&](double l, double t, bool top, bool left) {
		QPainterPath mp;
		if (top && left) {
			mp.moveTo(l, t + markSize);
			mp.lineTo(l, t);
			mp.lineTo(l + markSize, t);
		} else if (top && !left) {
			mp.moveTo(l - markSize, t);
			mp.lineTo(l, t);
			mp.lineTo(l, t + markSize);
		} else if (!top && left) {
			mp.moveTo(l, t - markSize);
			mp.lineTo(l, t);
			mp.lineTo(l + markSize, t);
		} else {
			mp.moveTo(l - markSize, t);
			mp.lineTo(l, t);
			mp.lineTo(l, t - markSize);
		}
		p.drawPath(mp);
	};

	drawCornerMark(r.left() + markOffset, r.top() + markOffset, true, true);
	drawCornerMark(r.right() - markOffset + 1, r.top() + markOffset, true, false);
	drawCornerMark(r.left() + markOffset, r.bottom() - markOffset + 1, false, true);
	drawCornerMark(r.right() - markOffset + 1, r.bottom() - markOffset + 1, false, false);
}

void Tile::setTileTitle(const QString &title) {
	m_title = title;
	m_titleLabel->setText(title);
	updateDerivedColors();
}

void Tile::setTileContent(const QString &content) {
	m_content = content;
	if (m_content.isEmpty()) {
		m_contentLabel->setText(QString());
		return;
	}

	auto& p = FloralTheme::instance().current();
	bool dark = p.paper.lightness() < 128;

	QString borderColor, altBg, textSoft, codeTxt, preBg, preTxt;
	QString link, quoteBorder, quoteText, h1Line, hrBg;

	if (dark) {
		borderColor   = "#21262d";
		altBg        = "#161b22";
		textSoft     = "#8b949e";
		codeTxt      = "#c9d1d9";
		preBg        = "#161b22";
		preTxt       = "#c9d1d9";
		link         = "#58a6ff";
		quoteBorder  = "#21262d";
		quoteText    = "#8b949e";
		h1Line       = "#21262d";
		hrBg         = "#21262d";
	} else {
		borderColor   = "#ddd";
		altBg        = "#f0f0f0";
		textSoft     = "#555555";
		codeTxt      = "#333333";
		preBg        = "#f8f8f8";
		preTxt       = "#333333";
		link         = "#4183C4";
		quoteBorder  = "#ddd";
		quoteText    = "#777777";
		h1Line       = "#eee";
		hrBg         = "#e7e7e7";
	}

	QString html = md4cToHtml(m_content,
		borderColor, altBg, textSoft,
		codeTxt, preBg, preTxt,
		link, quoteBorder, quoteText,
		h1Line, hrBg);
	m_contentLabel->setText(html);
}

void Tile::setTileColor(const QColor &color) {
	m_bgColor = color;
	updateDerivedColors();
}

void Tile::setTileFontSize(int fontSize) {
	m_fontSize = fontSize;
}

void Tile::updateDerivedColors() {
	bool light = ColorUtils::isLight(m_bgColor);
	QColor mixTarget = light ? QColor("#1a1a18") : QColor("#ffffff");
	m_borderColor  = ColorUtils::mixAlpha(m_bgColor, mixTarget, 0.18, 0.3);
	m_cornerColor  = ColorUtils::mixAlpha(m_bgColor, mixTarget, 0.30, 0.26);
	m_titleColor   = ColorUtils::mixAlpha(m_bgColor, mixTarget, 0.40, 0.5);

	QPalette pal = m_scroll->palette();
	pal.setColor(QPalette::Window, m_bgColor);
	pal.setColor(QPalette::Mid, m_bgColor);
	m_scroll->setPalette(pal);
	m_scroll->setBackgroundRole(QPalette::Window);

	QFont f("Noto Serif SC", m_fontSize + 1);
	f.setBold(true);
	m_titleLabel->setFont(f);
	m_titleLabel->setText(m_title);
	m_titleLabel->setStyleSheet(QString(
		"background: transparent; border: none; color: rgba(%1,%2,%3,%4);")
		.arg(m_titleColor.red()).arg(m_titleColor.green()).arg(m_titleColor.blue())
		.arg(m_titleColor.alphaF()));
}

void Tile::resizeEvent(QResizeEvent *event) {
	QWidget::resizeEvent(event);
	if (m_contentWidget && m_viewport) {
		m_contentWidget->setMinimumWidth(m_viewport->width());
		m_contentWidget->setMaximumWidth(m_viewport->width());
	}
}

void Tile::refreshForThemeChange() {
	auto& p = FloralTheme::instance().current();
	setTileColor(p.paper);
	setTileContent(m_content);
	update();
}
