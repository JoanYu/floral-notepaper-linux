#pragma once

#include <QWidget>
#include <QColor>

class NoteIndicator : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
public:
    explicit NoteIndicator(QWidget *parent = nullptr);
    QColor color() const;
    void setColor(const QColor &c);

protected:
    void paintEvent(QPaintEvent *event) override;

signals:
    void colorChanged(const QColor &);

private:
    QColor m_color;
};
