#ifndef NOTEPADWINDOW_H
#define NOTEPADWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include "services/notestore.h"
#include "services/configmanager.h"
#include "widgets/tile.h"

// Open mode: "new" = quick note, "open" = open existing note
class NotePadWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit NotePadWindow(QWidget *parent = nullptr);
    void initialize(NoteStore *store, ConfigManager *configMgr,
                    const QString &noteId = {}, const QString &surfaceMode = "pad");

signals:
    void requestClose();

protected:
    void closeEvent(QCloseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void buildUI();
    QWidget *buildMiniBar();
    void buildNewMode(QVBoxLayout *parent);
    void buildOpenMode(QVBoxLayout *parent);
    void rebuildNoteList();
    void loadNote(const QString &id);
    void saveNote();
    void toggleTileMode();
    void setSurfaceMode(const QString &mode);
    void setOpenMode(const QString &mode);
    void updateStatus();
    void clearDraft();

    NoteStore *m_store = nullptr;
    ConfigManager *m_configMgr = nullptr;
    AppConfig m_config;
    QPoint m_dragPos;

    QWidget *m_newModeWidget = nullptr;
    QWidget *m_openModeWidget = nullptr;
    QWidget *m_padWidget = nullptr;
    QWidget *m_tileWidget = nullptr;

    // New mode widgets
    QLineEdit *m_titleEdit = nullptr;
    QTextEdit *m_editor = nullptr;
    QLabel *m_statusLabel = nullptr;

    // Open mode widgets
    QWidget *m_openListContent = nullptr;
    QVBoxLayout *m_openListLayout = nullptr;

    // Tile
    Tile *m_tile = nullptr;

    QString m_noteId;
    QString m_surfaceMode;   // "pad" or "tile"
    QString m_openMode;      // "new" or "open"
    bool m_dirty = false;
};

#endif
