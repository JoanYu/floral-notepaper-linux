#include "styledcheckbox.h"
#include "app/theme.h"
#include <QPainter>
#include <QStyleOptionButton>
#include <QStylePainter>

StyledCheckBox::StyledCheckBox(const QString &text, QWidget *parent)
    : QCheckBox(text, parent) {}

void StyledCheckBox::paintEvent(QPaintEvent *) {
    QStylePainter sp(this);
    QStyleOptionButton opt;
    opt.initFrom(this);
    opt.text = text();
    opt.state |= (isChecked() ? QStyle::State_On : QStyle::State_Off);
    if (isDown()) opt.state |= QStyle::State_Sunken;
    if (isEnabled()) opt.state |= QStyle::State_Enabled;

    // Draw text (delegate to style, but we'll draw it ourselves)
    // Get indicator rect
    QRect indicator = style()->subElementRect(QStyle::SE_CheckBoxIndicator, &opt, this);

    auto &pal = FloralTheme::instance().current();

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Indicator background & border
    QRectF r = QRectF(indicator).adjusted(1, 1, -1, -1);
    bool hover = opt.state & QStyle::State_MouseOver;
    QColor borderCol = isChecked() ? pal.bamboo : (hover ? pal.bamboo : pal.inkFaint);
    if (isChecked() && hover) borderCol = pal.bambooLight;

    p.setPen(QPen(borderCol, 2));
    p.setBrush(pal.paperWarm);
    p.drawRoundedRect(r, 4, 4);

    // Inner fill when checked — 4x4 centered
    if (isChecked()) {
        QColor fill = hover ? pal.bambooLight : pal.bamboo;
        p.setPen(Qt::NoPen);
        p.setBrush(fill);
        QRectF inner = r.adjusted(4, 4, -4, -4);  // 12-2=10, inner 4x4 → adjust 3? Let me recalc
        // r is 12x12 adjusted by 1 = 10x10. Want 4x4 inner → adjust by (10-4)/2 = 3
        QRectF inner2 = r.adjusted(3, 3, -3, -3);
        p.drawRoundedRect(inner2, 2, 2);
    }

    // Draw text
    QRect textRect = style()->subElementRect(QStyle::SE_CheckBoxContents, &opt, this);
    p.setPen(pal.inkSoft);
    QFont f = font();
    f.setPixelSize(12);
    p.setFont(f);
    p.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text());
}
