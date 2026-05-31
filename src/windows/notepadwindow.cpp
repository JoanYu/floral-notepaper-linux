#include "notepadwindow.h"
#include "app/theme.h"
#include "widgets/slidingbuttongroup.h"
#include <QPushButton>
#include <QTextEdit>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCloseEvent>
#include <QMouseEvent>
#include <QWindow>
#include <QStackedLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QRegularExpression>
#include <QStyle>
#include <QMenu>
#include <QResizeEvent>

NotePadWindow::NotePadWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    resize(420, 320);
    buildUI();
}

void NotePadWindow::buildUI() {
    auto *central = new QWidget;
    central->setObjectName("notepadCentral");
    setCentralWidget(central);
    auto *mainVBox = new QVBoxLayout(central);
    mainVBox->setContentsMargins(0, 0, 0, 0);
    mainVBox->setSpacing(0);

    mainVBox->addWidget(buildMiniBar());

    // Separator line — Tauri: mx-4 mt-1 h-px bg-paper-deep/50
    auto *sep = new QWidget;
    sep->setFixedHeight(1);
    sep->setObjectName("padSeparator");
    mainVBox->addWidget(sep);

    // Surface mode stack (pad vs tile)
    auto *surfaceStack = new QStackedLayout;

    // === PAD surface ===
    m_padWidget = new QWidget;
    auto *padLayout = new QVBoxLayout(m_padWidget);
    padLayout->setContentsMargins(0, 0, 0, 0);
    padLayout->setSpacing(0);

    // Open mode stack (new vs open)
    auto *modeStack = new QStackedLayout;
    modeStack->setContentsMargins(0, 0, 0, 0);

    auto *newModeContainer = new QWidget;
    auto *newLayout = new QVBoxLayout(newModeContainer);
    newLayout->setContentsMargins(0, 0, 0, 0);
    buildNewMode(newLayout);
    modeStack->addWidget(newModeContainer);
    m_newModeWidget = newModeContainer;

    auto *openModeContainer = new QWidget;
    auto *openLayout = new QVBoxLayout(openModeContainer);
    openLayout->setContentsMargins(0, 0, 0, 0);
    buildOpenMode(openLayout);
    modeStack->addWidget(openModeContainer);
    m_openModeWidget = openModeContainer;

    padLayout->addLayout(modeStack, 1);
    surfaceStack->addWidget(m_padWidget);

    // === TILE surface ===
    m_tileWidget = new QWidget;
    auto *tileLayout = new QVBoxLayout(m_tileWidget);
    tileLayout->setContentsMargins(0, 0, 0, 0);
    auto& p = FloralTheme::instance().current();
    m_tile = new Tile;
    m_tile->setTileColor(p.paper);
    tileLayout->addWidget(m_tile, 1);
    connect(&FloralTheme::instance(), &FloralTheme::themeChanged, m_tile, &Tile::refreshForThemeChange);
    surfaceStack->addWidget(m_tileWidget);

    mainVBox->addLayout(surfaceStack, 1);
    setOpenMode("new");
    m_surfaceMode = "pad";
}

QWidget *NotePadWindow::buildMiniBar() {
    auto *bar = new QWidget;
    bar->setObjectName("padMiniBar");
    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(8, 10, 8, 0);
    layout->setSpacing(8);

    // New + Open on left
    auto *newBtn = new QPushButton;
    newBtn->setIcon(FloralTheme::icon(":/icons/plus.svg"));
    newBtn->setIconSize(QSize(14, 14));
    newBtn->setFixedSize(28, 28);
    newBtn->setObjectName("padActionBtn");
    newBtn->setFlat(true);
    newBtn->setToolTip("新建");
    newBtn->setCursor(Qt::PointingHandCursor);
    connect(newBtn, &QPushButton::clicked, this, [this]() { setOpenMode("new"); });
    layout->addWidget(newBtn);

    auto *openBtn = new QPushButton;
    openBtn->setIcon(FloralTheme::icon(":/icons/notepad.svg"));
    openBtn->setIconSize(QSize(14, 14));
    openBtn->setFixedSize(28, 28);
    openBtn->setObjectName("padActionBtn");
    openBtn->setFlat(true);
    openBtn->setToolTip("打开笔记");
    openBtn->setCursor(Qt::PointingHandCursor);
    connect(openBtn, &QPushButton::clicked, this, [this]() { setOpenMode("open"); });
    layout->addWidget(openBtn);

    layout->addStretch();

    // Pin + Close on right
    auto *tileBtn = new QPushButton;
    tileBtn->setIcon(FloralTheme::icon(":/icons/pin.svg"));
    tileBtn->setIconSize(QSize(14, 14));
    tileBtn->setFixedSize(28, 28);
    tileBtn->setObjectName("padActionBtn");
    tileBtn->setFlat(true);
    tileBtn->setToolTip("转为磁贴");
    tileBtn->setCursor(Qt::PointingHandCursor);
    connect(tileBtn, &QPushButton::clicked, this, &NotePadWindow::toggleTileMode);
    layout->addWidget(tileBtn);

    auto *closeBtn = new QPushButton;
    closeBtn->setIcon(FloralTheme::icon(":/icons/x.svg"));
    closeBtn->setIconSize(QSize(13, 13));
    closeBtn->setFixedSize(28, 28);
    closeBtn->setObjectName("padCloseBtn");
    closeBtn->setFlat(true);
    closeBtn->setCursor(Qt::PointingHandCursor);
    connect(closeBtn, &QPushButton::clicked, this, &QMainWindow::close);
    layout->addWidget(closeBtn);

    return bar;
}

void NotePadWindow::buildNewMode(QVBoxLayout *parent) {
    // Tauri: px-4 pt-3 pb-2 flex flex-col flex-1
    parent->setContentsMargins(8, 12, 8, 6);

    // Title input
    m_titleEdit = new QLineEdit;
    m_titleEdit->setObjectName("padTitleInput");
    m_titleEdit->setPlaceholderText("标题（可选）");
    connect(m_titleEdit, &QLineEdit::textChanged, this, [this]() {
        m_dirty = true;
    });
    parent->addWidget(m_titleEdit);

    // Content textarea — flex-1, wrapped in canvas
    auto *canvas = new QWidget;
    canvas->setObjectName("padCanvas");
    auto *canvasLayout = new QVBoxLayout(canvas);
    canvasLayout->setContentsMargins(8, 8, 0, 0);
    m_editor = new QTextEdit;
    m_editor->setObjectName("padContentArea");
    m_editor->setPlaceholderText("写点什么……");
    m_editor->setFrameShape(QFrame::NoFrame);
    m_editor->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_editor, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos){
        QMenu menu;
        menu.addAction("剪切", this, [this](){ m_editor->cut(); });
        menu.addAction("复制", this, [this](){ m_editor->copy(); });
        menu.addAction("粘贴", this, [this](){ m_editor->paste(); });
        menu.exec(m_editor->viewport()->mapToGlobal(pos));
    });
    connect(m_editor, &QTextEdit::textChanged, this, [this]() {
        m_dirty = true;
        updateStatus();
        if (m_config.noteSurfaceAutoSave) {
            QTimer::singleShot(900, this, [this]() {
                if (m_dirty && m_openMode == "new") saveNote();
            });
        }
    });
    canvasLayout->addWidget(m_editor);
    parent->addWidget(canvas, 1);

    // Bottom bar — Tauri: pt-2 border-t border-paper-deep/30
    auto *bottomBar = new QWidget;
    bottomBar->setObjectName("padBottomBar");
    auto *bottomL = new QHBoxLayout(bottomBar);
    bottomL->setContentsMargins(0, 0, 0, 2);
    bottomL->setSpacing(8);

    m_statusLabel = new QLabel("0 字 · 空");
    m_statusLabel->setObjectName("padStatusLabel");
    bottomL->addWidget(m_statusLabel);
    bottomL->addStretch();

    auto *clearBtn = new QPushButton("清空");
    clearBtn->setObjectName("padClearBtn");
    clearBtn->setFixedHeight(30);
    clearBtn->setFlat(true);
    clearBtn->setCursor(Qt::PointingHandCursor);
    connect(clearBtn, &QPushButton::clicked, this, &NotePadWindow::clearDraft);
    bottomL->addWidget(clearBtn);

    auto *saveBtn = new QPushButton("保存");
    saveBtn->setObjectName("padSaveBtn");
    saveBtn->setFixedHeight(30);
    saveBtn->setFlat(true);
    saveBtn->setCursor(Qt::PointingHandCursor);
    connect(saveBtn, &QPushButton::clicked, this, &NotePadWindow::saveNote);
    bottomL->addWidget(saveBtn);

    parent->addWidget(bottomBar);
}

void NotePadWindow::buildOpenMode(QVBoxLayout *parent) {
    // Tauri: p-2 flex-1 min-h-0 overflow-y-auto
    parent->setContentsMargins(16, 8, 2, 8);

    auto *scroll = new QScrollArea;
    scroll->setObjectName("padOpenScroll");
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(
        "QScrollBar:vertical{width:8px;border:none;background:transparent;}"
        "QScrollBar::handle:vertical{border-radius:4px;min-height:40px;}"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0px;border:none;}");

    m_openListContent = new QWidget;
    m_openListContent->setObjectName("padOpenList");
    m_openListLayout = new QVBoxLayout(m_openListContent);
    m_openListLayout->setContentsMargins(0, 0, 0, 0);
    m_openListLayout->setSpacing(2);

    scroll->setWidget(m_openListContent);
    parent->addWidget(scroll, 1);
}

void NotePadWindow::rebuildNoteList() {
    if (!m_store) return;
    QLayoutItem *child;
    while ((child = m_openListLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }

    auto notes = m_store->listNotes();
    for (auto &n : notes) {
        auto *w = new QPushButton;
        w->setObjectName("noteItem");
        w->setFlat(true);
        w->setCursor(Qt::PointingHandCursor);
        w->setFixedHeight(72);
        auto *outer = new QVBoxLayout(w);
        outer->setContentsMargins(20,8,0,8);
        outer->setSpacing(4);

        // Indicator bar — same as main sidebar
        auto *indicator = new QWidget(w);
        indicator->setObjectName("noteIndicator");
        indicator->setFixedSize(4, 40);
        indicator->move(6, 16);
        indicator->setProperty("indicatorOn", false);

        // Row 1: title only
        auto *row1 = new QHBoxLayout;
        row1->setSpacing(8);
        auto *title = new QLabel(n.title.isEmpty() ? "未命名" : n.title);
        title->setObjectName("noteTitle");
        title->setWordWrap(false);
        title->setTextFormat(Qt::PlainText);
        row1->addWidget(title, 1);
        outer->addLayout(row1);

        // Row 2: preview — single line
        auto *preview = new QLabel(n.preview.isEmpty() ? "空白笔记" : n.preview);
        preview->setObjectName("notePreview");
        preview->setWordWrap(false);
        preview->setTextFormat(Qt::PlainText);
        preview->setMaximumHeight(16);
        outer->addWidget(preview);

        // Larger gap to row 3
        outer->addSpacing(6);

        // Row 3: date + time + word count
        auto *row3 = new QHBoxLayout;
        row3->setSpacing(4);
        auto *createdDate = new QLabel(n.createdAt.toString("MM-dd"));
        createdDate->setObjectName("noteDate");
        row3->addWidget(createdDate);
        auto *createdTime = new QLabel(n.createdAt.toString("hh:mm"));
        createdTime->setObjectName("noteTime");
        row3->addWidget(createdTime);
        auto *createdLabel = new QLabel("创建");
        createdLabel->setObjectName("noteDate");
        row3->addWidget(createdLabel);
        auto *dot1 = new QLabel("·");
        dot1->setObjectName("noteDot");
        row3->addWidget(dot1);
        auto *modDate = new QLabel(n.updatedAt.toString("MM-dd"));
        modDate->setObjectName("noteDate");
        row3->addWidget(modDate);
        auto *modTime = new QLabel(n.updatedAt.toString("hh:mm"));
        modTime->setObjectName("noteTime");
        row3->addWidget(modTime);
        auto *modLabel = new QLabel("修改");
        modLabel->setObjectName("noteDate");
        row3->addWidget(modLabel);
        auto *dot = new QLabel("·");
        dot->setObjectName("noteDot");
        row3->addWidget(dot);
        auto *wc = new QLabel(QString("%1 字").arg(n.wordCount));
        wc->setObjectName("noteWordCount");
        row3->addWidget(wc);
        row3->addStretch();
        outer->addLayout(row3);

        connect(w, &QPushButton::clicked, this, [this, id = n.id]() { loadNote(id); });
        m_openListLayout->addWidget(w);
    }

    if (notes.isEmpty()) {
        auto *empty = new QLabel("还没有可打开的笔记");
        empty->setObjectName("padEmptyLabel");
        empty->setAlignment(Qt::AlignCenter);
        m_openListLayout->addWidget(empty);
    }
    m_openListLayout->addStretch();
}

void NotePadWindow::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && event->pos().y() < 36) {
        if (auto *wh = windowHandle()) { wh->startSystemMove(); return; }
    }
    QMainWindow::mousePressEvent(event);
}

void NotePadWindow::mouseMoveEvent(QMouseEvent *event) {
    QMainWindow::mouseMoveEvent(event);
}

void NotePadWindow::initialize(NoteStore *store, ConfigManager *configMgr,
                                const QString &noteId, const QString &surfaceMode) {
    m_store = store;
    m_configMgr = configMgr;
    m_config = configMgr->load();

    if (!noteId.isEmpty()) {
        loadNote(noteId);
    } else {
        m_openMode = "new";
        setOpenMode("new");
    }
    setSurfaceMode(surfaceMode);
    updateStatus();
}

void NotePadWindow::loadNote(const QString &id) {
    if (!m_store) return;
    try {
        auto note = m_store->readNote(id);
        m_noteId = id;
        m_editor->blockSignals(true);
        m_editor->setPlainText(note.content);
        m_editor->blockSignals(false);
        m_titleEdit->blockSignals(true);
        m_titleEdit->setText(note.title);
        m_titleEdit->setCursorPosition(0);
        m_titleEdit->blockSignals(false);
        m_tile->setTileTitle(note.title);
        m_tile->setTileContent(note.content);
        m_dirty = false;
        setOpenMode("new");
        setSurfaceMode("pad");
        updateStatus();
    } catch (...) {}
}

void NotePadWindow::saveNote() {
    if (!m_store) return;
    SaveNoteRequest req;
    req.title = m_titleEdit->text().trimmed().isEmpty() ? "便签" : m_titleEdit->text().trimmed();
    req.content = m_editor->toPlainText();
    try {
        if (m_noteId.isEmpty()) {
            auto note = m_store->createNote(req);
            m_noteId = note.id;
        } else {
            m_store->updateNote(m_noteId, req);
        }
        m_dirty = false;
        m_tile->setTileTitle(req.title);
        m_tile->setTileContent(req.content);
        updateStatus();
    } catch (...) {}
}

void NotePadWindow::toggleTileMode() {
    if (m_dirty) saveNote();
    setSurfaceMode(m_surfaceMode == "pad" ? "tile" : "pad");
}

void NotePadWindow::setSurfaceMode(const QString &mode) {
    m_surfaceMode = mode;
    auto *mainVBox = qobject_cast<QVBoxLayout*>(centralWidget()->layout());
    if (!mainVBox) return;
    // surface stack is at index 2 (miniBar 0, separator 1)
    auto *stack = qobject_cast<QStackedLayout*>(mainVBox->itemAt(2)->layout());
    if (!stack) return;

    if (mode == "tile") {
        stack->setCurrentWidget(m_tileWidget);
    } else {
        stack->setCurrentWidget(m_padWidget);
    }
}

void NotePadWindow::setOpenMode(const QString &mode) {
    m_openMode = mode;
    // Find the mode stack inside padWidget
    auto *padLayout = m_padWidget->layout();
    if (!padLayout) return;
    auto *modeStack = qobject_cast<QStackedLayout*>(padLayout->itemAt(0)->layout());
    if (!modeStack) return;

    if (mode == "new") {
        modeStack->setCurrentWidget(m_newModeWidget);
    } else {
        modeStack->setCurrentWidget(m_openModeWidget);
        m_openModeWidget->show();
        rebuildNoteList();
    }
}

void NotePadWindow::updateStatus() {
    if (!m_editor || !m_statusLabel) return;
    int chars = m_editor->toPlainText().length();
    QString state = m_dirty ? "未保存" : (m_noteId.isEmpty() ? "空" : "已打开");
    m_statusLabel->setText(QString("%1 字 · %2").arg(chars).arg(state));
}

void NotePadWindow::clearDraft() {
    m_titleEdit->clear();
    m_editor->clear();
    m_noteId.clear();
    m_dirty = false;
    updateStatus();
}

void NotePadWindow::closeEvent(QCloseEvent *event) {
    if (m_dirty) saveNote();
    QMainWindow::closeEvent(event);
}

void NotePadWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
}
