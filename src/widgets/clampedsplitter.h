// ClampedSplitter — custom handle that prevents panels from going below their minimumWidth
#ifndef CLAMPEDSPLITTER_H
#define CLAMPEDSPLITTER_H

#include <QSplitter>
#include <QMouseEvent>

class ClampedSplitterHandle : public QSplitterHandle {
    Q_OBJECT
public:
    ClampedSplitterHandle(Qt::Orientation orient, QSplitter *parent)
        : QSplitterHandle(orient, parent), m_orientation(orient) {
        setFixedWidth(4);
        setCursor(orient == Qt::Horizontal ? Qt::SplitHCursor : Qt::SplitVCursor);
        m_dragging = false;
    }

protected:
    void mousePressEvent(QMouseEvent *e) override {
        if (e->button() == Qt::LeftButton) {
            m_dragging = true;
            m_anchor = m_orientation == Qt::Horizontal ? e->pos().x() : e->pos().y();
        }
        QSplitterHandle::mousePressEvent(e);
    }

    void mouseMoveEvent(QMouseEvent *e) override {
        if (m_dragging && (e->buttons() & Qt::LeftButton)) {
            QSplitter *sp = splitter();
            if (!sp) return;

            QList<int> sizes = sp->sizes();
            if (sizes.size() != 2) return;

            int min0 = sp->widget(0)->minimumWidth();
            int min1 = sp->widget(1)->minimumWidth();
            int total = sizes[0] + sizes[1];

            int pos;
            if (m_orientation == Qt::Horizontal) {
                pos = e->globalPosition().x() - sp->mapToGlobal(QPoint()).x() - m_anchor;
            } else {
                pos = e->globalPosition().y() - sp->mapToGlobal(QPoint()).y() - m_anchor;
            }

            pos = qBound(min0, pos, total - min1);

            int newFirst = pos;
            int newSecond = total - pos;
            sp->blockSignals(true);
            sp->setSizes({newFirst, newSecond});
            sp->blockSignals(false);
            return;
        }
        QSplitterHandle::mouseMoveEvent(e);
    }

    void mouseReleaseEvent(QMouseEvent *e) override {
        m_dragging = false;
        QSplitterHandle::mouseReleaseEvent(e);
    }

private:
    Qt::Orientation m_orientation;
    bool m_dragging;
    int m_anchor = 0;
};

class ClampedSplitter : public QSplitter {
    Q_OBJECT
public:
    explicit ClampedSplitter(Qt::Orientation orientation, QWidget *parent = nullptr)
        : QSplitter(orientation, parent) {
        setHandleWidth(4);
        setCollapsible(0, false);
        setCollapsible(1, false);
    }

protected:
    QSplitterHandle *createHandle() override {
        return new ClampedSplitterHandle(orientation(), this);
    }
};

#endif