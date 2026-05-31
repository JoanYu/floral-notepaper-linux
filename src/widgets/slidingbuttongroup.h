// SlidingButtonGroup — 滑动按钮组 (对应原 SlidingButtonGroup.tsx)
#ifndef SLIDINGBUTTONGROUP_H
#define SLIDINGBUTTONGROUP_H

#include <QWidget>
#include <QStringList>
#include <QVector>

struct ButtonOption {
    QString value;
    QString label;
};

class SlidingButtonGroup : public QWidget {
    Q_OBJECT
public:
    explicit SlidingButtonGroup(QWidget *parent = nullptr);

    void setOptions(const QVector<ButtonOption> &options);
    void setCurrentValue(const QString &value);
    QString currentValue() const { return m_currentValue; }

signals:
    void valueChanged(const QString &value);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void updateHighlight();
    int buttonIndexAt(const QPoint &pos) const;

    QVector<ButtonOption> m_options;
    QString m_currentValue;
    QRectF m_highlightRect;
    QVector<QRectF> m_buttonRects;
};

#endif // SLIDINGBUTTONGROUP_H
