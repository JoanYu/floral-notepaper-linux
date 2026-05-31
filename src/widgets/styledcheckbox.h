#ifndef STYLEDCHECKBOX_H
#define STYLEDCHECKBOX_H

#include <QCheckBox>

class StyledCheckBox : public QCheckBox {
    Q_OBJECT
public:
    explicit StyledCheckBox(const QString &text, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *e) override;
};

#endif
