// ContextMenu — 右键菜单 (替代 DOM-based context menu)
#ifndef CONTEXTMENU_H
#define CONTEXTMENU_H

#include <QMenu>

class ContextMenu : public QMenu {
    Q_OBJECT
public:
    explicit ContextMenu(QWidget *parent = nullptr);

    void addActionItem(const QString &text, const QString &shortcut = {},
                       bool danger = false, bool disabled = false);

signals:
    void actionTriggered(const QString &actionId);

private:
    QStringList m_actionIds;
};

#endif // CONTEXTMENU_H
