#pragma once

#include <QPushButton>
#include <QColor>

class NoteItemButton : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(QColor indicatorColor READ indicatorColor WRITE setIndicatorColor)
public:
    explicit NoteItemButton(QWidget *parent = nullptr);
    void setSelected(bool selected);
    QColor indicatorColor() const { return m_indicatorColor; }
    void setIndicatorColor(const QColor &color);
protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
private:
    QLabel *m_indicator = nullptr;
    QColor m_indicatorColor;
    QColor m_normalColor;
    QColor m_hoverColor;
    QColor m_selectedColor;
    bool m_isSelected = false;
    void updateIndicatorStyle();
};
