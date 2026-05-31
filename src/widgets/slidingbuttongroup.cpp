#include "slidingbuttongroup.h"
#include "app/theme.h"
#include <QPainter>
#include <QMouseEvent>

SlidingButtonGroup::SlidingButtonGroup(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(32);
    setMouseTracking(true);
}

void SlidingButtonGroup::setOptions(const QVector<ButtonOption> &options) {
    m_options = options;
    m_buttonRects.clear();
    update();
}

void SlidingButtonGroup::setCurrentValue(const QString &value) {
    m_currentValue = value;
    updateHighlight();
    update();
}

void SlidingButtonGroup::resizeEvent(QResizeEvent *) {
    updateHighlight();
}

void SlidingButtonGroup::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    auto &pt = FloralTheme::instance().current();
    QRectF r = rect().adjusted(1, 1, -1, -1);

    // 背景 — Tauri: bg-paper-warm/60 border border-paper-deep rounded-lg
    p.setPen(QPen(pt.paperDeep, 1));
    p.setBrush(QColor(pt.paperWarm.red(), pt.paperWarm.green(), pt.paperWarm.blue(), 153));
    p.drawRoundedRect(r, 8, 8);

    // 先计算按钮区域，再绘制高亮滑块
    m_buttonRects.clear();
    if (!m_options.isEmpty()) {
        double pad = 2;
        double totalW = r.width() - pad * 2;
        double btnWidth = totalW / m_options.size();
        for (int i = 0; i < m_options.size(); ++i) {
            QRectF btnRect(r.left() + pad + i * btnWidth, r.top() + 2,
                           btnWidth, r.height() - 4);
            m_buttonRects.append(btnRect);
        }
    }

    // 更新高亮滑块位置（m_buttonRects 已就绪）
    updateHighlight();

    // 高亮滑块 — Tauri: bg-cloud shadow
    if (!m_highlightRect.isEmpty()) {
        p.setBrush(pt.cloud);
        p.setPen(QPen(QColor(0, 0, 0, 10), 1));
        p.drawRoundedRect(m_highlightRect, 6, 6);
    }

    // 按钮文字
    QFont f = font();
    f.setPixelSize(11);  // Tauri: text-[11px]
    p.setFont(f);

    for (int i = 0; i < m_options.size(); ++i) {
        bool selected = (m_options[i].value == m_currentValue);
        p.setPen(selected ? pt.bamboo : pt.inkGhost);
        p.drawText(m_buttonRects[i], Qt::AlignCenter, m_options[i].label);
    }
}

void SlidingButtonGroup::mouseReleaseEvent(QMouseEvent *event) {
    int idx = buttonIndexAt(event->pos());
    if (idx >= 0 && idx < m_options.size()) {
        QString val = m_options[idx].value;
        if (val != m_currentValue) {
            m_currentValue = val;
            updateHighlight();
            repaint();
            emit valueChanged(m_currentValue);
        }
    }
}

void SlidingButtonGroup::updateHighlight() {
    int idx = -1;
    for (int i = 0; i < m_options.size(); ++i) {
        if (m_options[i].value == m_currentValue) { idx = i; break; }
    }
    if (idx >= 0 && idx < m_buttonRects.size()) {
        m_highlightRect = m_buttonRects[idx];
    } else {
        m_highlightRect = QRectF();
    }
}

int SlidingButtonGroup::buttonIndexAt(const QPoint &pos) const {
    for (int i = 0; i < m_buttonRects.size(); ++i) {
        if (m_buttonRects[i].contains(pos))
            return i;
    }
    return -1;
}
