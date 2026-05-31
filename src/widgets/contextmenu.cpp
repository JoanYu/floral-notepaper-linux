#include "contextmenu.h"
#include <QAction>

ContextMenu::ContextMenu(QWidget *parent)
    : QMenu(parent)
{
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
}

void ContextMenu::addActionItem(const QString &text, const QString &shortcut,
                                 bool danger, bool disabled) {
    auto *action = addAction(text);
    action->setShortcut(QKeySequence(shortcut));
    action->setEnabled(!disabled);
    action->setData(text);

    if (danger) {
        action->setIcon(QIcon()); // placeholder
    }

    connect(action, &QAction::triggered, this, [this, action]() {
        emit actionTriggered(action->text());
    });
}
