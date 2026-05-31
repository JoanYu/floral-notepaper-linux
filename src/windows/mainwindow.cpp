#include "mainwindow.h"
#include "notepadwindow.h"
#include "widgets/markdownpreview.h"
#include "widgets/slidingbuttongroup.h"
#include "widgets/clampedsplitter.h"
#include "settingspanel.h"
#include "app/theme.h"
#include <QSplitter>
#include <QTextEdit>
#include <QTextDocument>
#include <QTextCursor>
#include <QScrollArea>
#include <QScrollBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QMouseEvent>
#include <QWindow>
#include <QIcon>
#include <QMenu>
#include <QInputDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QStyle>
#include <QPropertyAnimation>

// ── CategoryHeader ──
CategoryHeader::CategoryHeader(const QString &name, bool isUncategorized, QWidget *parent)
    : QWidget(parent), m_name(name), m_isUncategorized(isUncategorized)
{
    setObjectName("categoryHeader");
    setProperty("catExpanded", m_expanded);
    auto *lay = new QHBoxLayout(this);
    lay->setContentsMargins(10, 5, 10, 5);
    lay->setSpacing(6);

    m_arrow = new QPushButton;
    m_arrow->setObjectName("catArrow");
    m_arrow->setIcon(FloralTheme::icon(m_expanded ? ":/icons/chevron-down.svg" : ":/icons/chevron-right.svg"));
    m_arrow->setIconSize(QSize(14, 14));
    m_arrow->setFlat(true);
    m_arrow->setFixedSize(20, 20);
    m_arrow->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_arrow->setStyleSheet("background:transparent;");
    lay->addWidget(m_arrow);

    // Folder icon
    m_icon = new QPushButton;
    m_icon->setIcon(FloralTheme::icon(":/icons/folder.svg"));
    m_icon->setIconSize(QSize(12, 12));
    m_icon->setFlat(true);
    m_icon->setObjectName("catIcon");
    m_icon->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_icon->setStyleSheet("background:transparent;");
    lay->addWidget(m_icon);

    m_label = new QLabel(name.isEmpty() ? "未分类" : name);
    m_label->setObjectName("catName");
    lay->addWidget(m_label, 1);

    m_count = new QLabel("0");
    m_count->setObjectName("catCount");
    lay->addWidget(m_count);

    setCursor(Qt::PointingHandCursor);
}

void CategoryHeader::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::RightButton && !m_isUncategorized) {
        QMenu menu;
        menu.addAction("重命名", this, [this](){ emit renameRequested(m_name); });
        menu.addAction("删除分类", this, [this](){ emit deleteRequested(m_name); });
        menu.exec(e->globalPosition().toPoint());
        return;
    }
    m_expanded = !m_expanded;
    setProperty("catExpanded", m_expanded);
    style()->unpolish(this);
    style()->polish(this);
    if (m_arrow) m_arrow->setIcon(FloralTheme::icon(m_expanded ? ":/icons/chevron-down.svg" : ":/icons/chevron-right.svg"));
    emit toggled(m_name);
}

void CategoryHeader::setExpanded(bool expanded) {
    m_expanded = expanded;
    setProperty("catExpanded", expanded);
    if (m_arrow) m_arrow->setIcon(FloralTheme::icon(expanded ? ":/icons/chevron-down.svg" : ":/icons/chevron-right.svg"));
}

void CategoryHeader::setCount(int count) {
    if (m_count) m_count->setText(QString::number(count));
}

// ── MainWindow ──
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setWindowTitle("花笺");
    resize(1100, 720);
    setMinimumWidth(820);
    buildUI();
}

QWidget *MainWindow::buildTitleBar() {
    auto *bar = new QWidget;
    bar->setObjectName("titleBar");
    bar->setFixedHeight(44);
    bar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto *l = new QHBoxLayout(bar);
    l->setContentsMargins(16,0,0,0);  // pl-4
    l->setSpacing(12);

    // Sidebar toggle — free-floating, positioned left of viewModeGroup in eventFilter
    m_sidebarToggleBtn = new QPushButton(bar);
    m_sidebarToggleBtn->setIcon(FloralTheme::icon(":/icons/sidebar.svg"));
    m_sidebarToggleBtn->setIconSize(QSize(14, 14));
    m_sidebarToggleBtn->setFlat(true);
    m_sidebarToggleBtn->setObjectName("titleActionBtn");
    m_sidebarToggleBtn->setToolTip("收起侧栏");
    m_sidebarToggleBtn->setFixedSize(40, 44);
    connect(m_sidebarToggleBtn, &QPushButton::clicked, this, &MainWindow::toggleSidebar);

    auto *t = new QLabel("花笺");
    t->setObjectName("appTitle");
    l->addWidget(t);

    auto *sep = new QLabel("—");
    sep->setObjectName("subSeparator");
    l->addWidget(sep);

    m_subTitleLabel = new QLabel("无标题笔记");
    m_subTitleLabel->setObjectName("subTitle");
    m_subTitleLabel->setTextFormat(Qt::PlainText);
    m_subTitleLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_subTitleLabel->setProperty("fullText", "无标题笔记");
    m_subTitleLabel->installEventFilter(this);
    l->addWidget(m_subTitleLabel, 1);

    l->addStretch();

    // Notepad quick-launch button
    auto *noteBtn = new QPushButton;
    noteBtn->setIcon(FloralTheme::icon(":/icons/notepad.svg"));
    noteBtn->setIconSize(QSize(14, 14));
    noteBtn->setFlat(true);
    noteBtn->setObjectName("titleActionBtn");
    noteBtn->setToolTip("快捷便签");
    connect(noteBtn, &QPushButton::clicked, this, [this](){
        auto *pad = new NotePadWindow;
        pad->initialize(m_store, m_configMgr, "", "pad");
        pad->show();
    });
    l->addWidget(noteBtn);

    // Settings button
    auto *setBtn = new QPushButton;
    setBtn->setIcon(FloralTheme::icon(":/icons/settings.svg"));
    setBtn->setIconSize(QSize(14, 14));
    setBtn->setFlat(true);
    setBtn->setObjectName("titleActionBtn");
    setBtn->setToolTip("设置");
    connect(setBtn, &QPushButton::clicked, this, &MainWindow::toggleSettings);
    l->addWidget(setBtn);

    // Separator before window controls
    auto *div = new QWidget;
    div->setObjectName("titleBarDivider");
    div->setFixedSize(1, 16);
    l->addWidget(div);

    // Window control buttons
    auto *min = new QPushButton;
    min->setIcon(FloralTheme::icon(":/icons/window-minimize.svg"));
    min->setIconSize(QSize(12, 12));
    min->setFlat(true);
    min->setObjectName("winCtrlBtn");
    connect(min, &QPushButton::clicked, this, &QMainWindow::showMinimized);
    l->addWidget(min);

    auto *max = new QPushButton;
    m_maxBtn = max;
    max->setIcon(FloralTheme::icon(":/icons/window-maximize.svg"));
    max->setIconSize(QSize(12, 12));
    max->setFlat(true);
    max->setObjectName("winCtrlBtn");
    connect(max, &QPushButton::clicked, this, [this](){
        isMaximized()?showNormal():showMaximized(); });
    l->addWidget(max);

    auto *cls = new QPushButton;
    cls->setIcon(FloralTheme::icon(":/icons/window-close.svg"));
    cls->setIconSize(QSize(12, 12));
    cls->setFlat(true);
    cls->setObjectName("closeBtn");
    connect(cls, &QPushButton::clicked, this, &QMainWindow::close);
    l->addWidget(cls);

    // Free-floating view mode switcher — centered via resize. Order: 编辑 | 预览 | 分栏
    m_viewModeGroup = new SlidingButtonGroup(bar);
    m_viewModeGroup->setOptions({{"edit","编辑"},{"preview","预览"},{"split","分栏"}});
    m_viewModeGroup->setCurrentValue("edit");
    m_viewModeGroup->setFixedWidth(200);
    connect(m_viewModeGroup, &SlidingButtonGroup::valueChanged, this, &MainWindow::onViewModeChanged);
    bar->installEventFilter(this);

    return bar;
}

void MainWindow::buildUI() {
    auto *c = new QWidget;
    c->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setCentralWidget(c);
    auto *mv = new QVBoxLayout(c);
    mv->setContentsMargins(0,0,0,0); mv->setSpacing(0);
    mv->addWidget(buildTitleBar());

    auto *body = new QWidget;
    body->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto *root = new QHBoxLayout(body);
    root->setContentsMargins(0,0,0,0); root->setSpacing(0);

    // ── 左侧边栏 (Tauri: w-[280px] bg-paper/40 border-r border-paper-deep/30) ──
    m_sidebarWidget = new QWidget;
    m_sidebarWidget->setObjectName("sidebarPanel");
    m_sidebarWidget->setMinimumWidth(0);
    m_sidebarWidget->setFixedWidth(m_sidebarWidth);
    auto *sbl = new QVBoxLayout(m_sidebarWidget);
    sbl->setContentsMargins(0,0,0,0); sbl->setSpacing(0);

    // Search box area (px-3 pt-3 pb-2)
    auto *searchArea = new QWidget;
    auto *searchAreaL = new QVBoxLayout(searchArea);
    searchAreaL->setContentsMargins(12,12,12,8); searchAreaL->setSpacing(0);
    m_searchBox = new QLineEdit;
    m_searchBox->setObjectName("searchBox");
    m_searchBox->setPlaceholderText("搜索笔记…");
    m_searchBox->setFixedHeight(32);
    m_searchBox->addAction(FloralTheme::icon(":/icons/search.svg"), QLineEdit::LeadingPosition);
    auto *clearAction = m_searchBox->addAction("✕", QLineEdit::TrailingPosition);
    clearAction->setVisible(false);
    connect(clearAction, &QAction::triggered, this, [this](){ m_searchBox->clear(); });
    connect(m_searchBox, &QLineEdit::textChanged, this, [this, clearAction](const QString &t){
        clearAction->setVisible(!t.isEmpty());
        onSearchChanged(t);
    });
    searchAreaL->addWidget(m_searchBox);
    sbl->addWidget(searchArea);

    // Scrollable note list (flex-1 overflow-y-auto px-2 pb-2)
    m_sidebarScroll = new QScrollArea;
    m_sidebarScroll->setObjectName("sidebarScroll");
    m_sidebarScroll->setWidgetResizable(true);
    m_sidebarScroll->setFrameShape(QFrame::NoFrame);
    m_sidebarScroll->setStyleSheet(
        "QScrollArea#sidebarScroll{padding-right:4px;}"
        "QScrollBar:vertical{width:8px;border:none;background:transparent;}"
        "QScrollBar::handle:vertical{border-radius:4px;min-height:40px;}"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0px;border:none;}");
    m_sidebarContent = new QWidget;
    m_sidebarLayout = new QVBoxLayout(m_sidebarContent);
    m_sidebarLayout->setContentsMargins(12,0,4,8);
    m_sidebarLayout->setSpacing(8);
    m_sidebarScroll->setWidget(m_sidebarContent);
    sbl->addWidget(m_sidebarScroll, 1);

    // Bottom action buttons — anchored to bottom, aligned with note items
    auto *actionsArea = new QWidget;
    auto *actionsL = new QVBoxLayout(actionsArea);
    actionsL->setContentsMargins(12, 8, 12, 12); actionsL->setSpacing(8);

    auto *noteCountLabel = new QLabel("0 篇笔记");
    noteCountLabel->setObjectName("sidebarNoteCount");
    actionsL->addWidget(noteCountLabel);

    auto *newBtn = new QPushButton("新建笔记");
    newBtn->setObjectName("sidebarNewBtn");
    newBtn->setFlat(true);
    newBtn->setCursor(Qt::PointingHandCursor);
    connect(newBtn, &QPushButton::clicked, this, &MainWindow::onNewNote);
    actionsL->addWidget(newBtn);

    auto *importBtn = new QPushButton("导入 Markdown");
    importBtn->setObjectName("sidebarImportBtn");
    importBtn->setFlat(true);
    importBtn->setCursor(Qt::PointingHandCursor);
    connect(importBtn, &QPushButton::clicked, this, [this](){
        QString path = QFileDialog::getOpenFileName(this, "导入 Markdown", QString(), "Markdown (*.md)");
        if (!path.isEmpty() && m_store) {
            m_store->importMarkdownFile(path, "");
            rebuildNoteList();
        }
    });
    actionsL->addWidget(importBtn);
    sbl->addWidget(actionsArea);

    root->addWidget(m_sidebarWidget);

    // 拖拽手柄 — 4px 宽，鼠标悬停变双向箭头
    m_sidebarHandle = new QWidget;
    m_sidebarHandle->setFixedWidth(4);
    m_sidebarHandle->setCursor(Qt::SplitHCursor);
    m_sidebarHandle->setObjectName("sidebarHandle");
    m_sidebarHandle->installEventFilter(this);
    root->addWidget(m_sidebarHandle);

    // ── 右侧 ──
    auto *rp = new QWidget;
    rp->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto *rpl = new QVBoxLayout(rp);
    rpl->setContentsMargins(0,0,0,0); rpl->setSpacing(0);

    // Title + meta row
    auto *titleRow = new QWidget;
    titleRow->setObjectName("editorMetaBar");
    auto *trl = new QVBoxLayout(titleRow);
    trl->setContentsMargins(12,16,12,8);
    m_titleEdit = new QLineEdit;
    m_titleEdit->setObjectName("editorTitleInput");
    m_titleEdit->setPlaceholderText("无标题笔记");
    connect(m_titleEdit, &QLineEdit::textChanged, this, &MainWindow::onContentChanged);
    trl->addWidget(m_titleEdit);

    auto *metaRow = new QHBoxLayout;
    metaRow->setSpacing(4);
    m_createdLabel = new QLabel("--");
    m_createdLabel->setObjectName("metaInfoLabel");
    metaRow->addWidget(m_createdLabel);
    auto *createdIcon = new QLabel("创建");
    createdIcon->setObjectName("metaInfoLabel");
    metaRow->addWidget(createdIcon);
    auto *dot0 = new QLabel("·");
    dot0->setObjectName("metaDot");
    metaRow->addWidget(dot0);
    m_dateLabel = new QLabel("--");
    m_dateLabel->setObjectName("metaInfoLabel");
    metaRow->addWidget(m_dateLabel);
    auto *modifiedIcon = new QLabel("修改");
    modifiedIcon->setObjectName("metaInfoLabel");
    metaRow->addWidget(modifiedIcon);
    auto *dot1 = new QLabel("·");
    dot1->setObjectName("metaDot");
    metaRow->addWidget(dot1);
    m_charCountLabel = new QLabel("0 字");
    m_charCountLabel->setObjectName("metaInfoLabel");
    metaRow->addWidget(m_charCountLabel);
    auto *dot2 = new QLabel("·");
    dot2->setObjectName("metaDot");
    metaRow->addWidget(dot2);
    m_metaStatusLabel = new QLabel("未保存");
    m_metaStatusLabel->setObjectName("saveStateLabel");
    metaRow->addWidget(m_metaStatusLabel);
    metaRow->addStretch();
    trl->addLayout(metaRow);
    rpl->addWidget(titleRow);

    // Format toolbar
    auto *ft = new QWidget;
    ft->setObjectName("formatToolbar");
    auto *ftl = new QHBoxLayout(ft);
    ftl->setContentsMargins(12,8,12,8);
    ftl->setSpacing(4);
    QVector<QPair<QString,QString>> btns = {
        {"B","bold"},{"I","italic"},{"H","heading"},{"—","hr"},
        {"•","ul"},{"1.","ol"},{"<>","code"},{"❝","quote"}};
    for (auto &b : btns) {
        auto *btn = new QPushButton(b.first);
        btn->setToolTip(b.second);
        btn->setFlat(true);
        btn->setFixedSize(28, 28);
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, this, [this, a=b.second](){ onFormatBtn(a); });
        ftl->addWidget(btn);
    }
    ftl->addStretch();

    // Separator before action icons
    auto *ftSep = new QWidget;
    ftSep->setObjectName("toolbarDivider");
    ftSep->setFixedSize(1, 16);
    ftl->addWidget(ftSep);

    // Save
    auto *saveBtn = new QPushButton;
    saveBtn->setIcon(FloralTheme::icon(":/icons/save.svg"));
    saveBtn->setIconSize(QSize(14, 14));
    saveBtn->setToolTip("保存");
    saveBtn->setFlat(true);
    saveBtn->setFixedSize(28, 28);
    saveBtn->setCursor(Qt::PointingHandCursor);
    saveBtn->setObjectName("toolbarSaveBtn");
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::onSaveNote);
    ftl->addWidget(saveBtn);

    // Undo
    auto *undoBtn = new QPushButton;
    undoBtn->setIcon(FloralTheme::icon(":/icons/undo.svg"));
    undoBtn->setIconSize(QSize(14, 14));
    undoBtn->setToolTip("撤销");
    undoBtn->setFlat(true);
    undoBtn->setFixedSize(28, 28);
    undoBtn->setCursor(Qt::PointingHandCursor);
    undoBtn->setObjectName("toolbarIconBtn");
    connect(undoBtn, &QPushButton::clicked, this, [this](){ if(m_editor) m_editor->undo(); });
    ftl->addWidget(undoBtn);

    // Redo
    auto *redoBtn = new QPushButton;
    redoBtn->setIcon(FloralTheme::icon(":/icons/redo.svg"));
    redoBtn->setIconSize(QSize(14, 14));
    redoBtn->setToolTip("重做");
    redoBtn->setFlat(true);
    redoBtn->setFixedSize(28, 28);
    redoBtn->setCursor(Qt::PointingHandCursor);
    redoBtn->setObjectName("toolbarIconBtn");
    connect(redoBtn, &QPushButton::clicked, this, [this](){ if(m_editor) m_editor->redo(); });
    ftl->addWidget(redoBtn);

    // Delete note
    auto *delBtn = new QPushButton;
    delBtn->setIcon(FloralTheme::icon(":/icons/trash.svg"));
    delBtn->setIconSize(QSize(14, 14));
    delBtn->setToolTip("删除笔记");
    delBtn->setFlat(true);
    delBtn->setFixedSize(28, 28);
    delBtn->setCursor(Qt::PointingHandCursor);
    delBtn->setObjectName("trashBtn");
    connect(delBtn, &QPushButton::clicked, this, &MainWindow::onDeleteNote);
    ftl->addWidget(delBtn);

    // Pin to tile
    auto *pinBtn = new QPushButton;
    pinBtn->setIcon(FloralTheme::icon(":/icons/pin.svg"));
    pinBtn->setIconSize(QSize(14, 14));
    pinBtn->setToolTip("钉为磁贴");
    pinBtn->setFlat(true);
    pinBtn->setFixedSize(28, 28);
    pinBtn->setCursor(Qt::PointingHandCursor);
    pinBtn->setObjectName("pinTileBtn");
    connect(pinBtn, &QPushButton::clicked, this, &MainWindow::onPinToTile);
    ftl->addWidget(pinBtn);

    rpl->addWidget(ft);

    m_splitter = new ClampedSplitter(Qt::Horizontal);
    m_splitter->setCollapsible(0, false);
    m_splitter->setCollapsible(1, false);
    m_splitter->setHandleWidth(4);

    // Editor canvas
    m_editorContainer = new QWidget;
    m_editorContainer->setObjectName("editorCanvas");
    m_editorContainer->setMinimumWidth(200);
    auto *el = new QVBoxLayout(m_editorContainer);
    el->setContentsMargins(0,0,0,0);
    m_editor = new QTextEdit;
    m_editor->setObjectName("editorTextArea");
    m_editor->setPlaceholderText("开始写作……");
    m_editor->setFrameShape(QFrame::NoFrame);
    m_editor->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_editor, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos){
        QMenu menu;
        menu.addAction("剪切", this, [this](){ m_editor->cut(); });
        menu.addAction("复制", this, [this](){ m_editor->copy(); });
        menu.addAction("粘贴", this, [this](){ m_editor->paste(); });
        menu.exec(m_editor->viewport()->mapToGlobal(pos));
    });
    { QFont f = m_editor->font(); f.setPixelSize(14); m_editor->setFont(f); }
    connect(m_editor, &QTextEdit::textChanged, this, &MainWindow::onContentChanged);
    el->addWidget(m_editor);
    m_splitter->addWidget(m_editorContainer);

    // Preview canvas
    m_previewContainer = new QWidget;
    m_previewContainer->setObjectName("previewCanvas");
    m_previewContainer->setMinimumWidth(200);
    auto *pl = new QVBoxLayout(m_previewContainer);
    pl->setContentsMargins(0,0,0,0);
    m_preview = new MarkdownPreview;
    pl->addWidget(m_preview);
    m_splitter->addWidget(m_previewContainer);
    m_previewContainer->hide();

    // Scroll sync between editor and preview (ratio-based, bidirectional, loop-safe)
    m_editor->verticalScrollBar()->setSingleStep(20);
    m_preview->verticalScrollBar()->setSingleStep(20);

    connect(m_editor->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int){
        if (m_syncingScroll || !m_preview->isVisible()) return;
        m_syncingScroll = true;
        QScrollBar *editorBar = m_editor->verticalScrollBar();
        QScrollBar *previewBar = m_preview->verticalScrollBar();
        int editorMax = editorBar->maximum();
        int previewMax = previewBar->maximum();
        if (editorMax <= 0 || previewMax <= 0) { m_syncingScroll = false; return; }
        double ratio = static_cast<double>(editorBar->value()) / editorMax;
        previewBar->setValue(static_cast<int>(ratio * previewMax));
        m_syncingScroll = false;
    });

    connect(m_preview->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int){
        if (m_syncingScroll) return;
        m_syncingScroll = true;
        QScrollBar *editorBar = m_editor->verticalScrollBar();
        QScrollBar *previewBar = m_preview->verticalScrollBar();
        int editorMax = editorBar->maximum();
        int previewMax = previewBar->maximum();
        if (editorMax <= 0 || previewMax <= 0) { m_syncingScroll = false; return; }
        double ratio = static_cast<double>(previewBar->value()) / previewMax;
        editorBar->setValue(static_cast<int>(ratio * editorMax));
        m_syncingScroll = false;
    });

    rpl->addWidget(m_splitter, 1);

    // Status bar — Tauri: h-7 bg-paper/30 border-t border-paper-deep/20
    auto *stBar = new QWidget;
    stBar->setObjectName("statusBar");
    auto *stl = new QHBoxLayout(stBar);
    stl->setContentsMargins(16,0,16,0);
    // Left: Ln N | Markdown
    m_lineCountLabel = new QLabel("Ln 0");
    stl->addWidget(m_lineCountLabel);
    auto *sep1 = new QLabel("|");
    stl->addWidget(sep1);
    auto *fmtLabel = new QLabel("Markdown");
    stl->addWidget(fmtLabel);
    stl->addStretch();
    // Right: UTF-8 | N KB
    auto *encLabel = new QLabel("UTF-8");
    stl->addWidget(encLabel);
    auto *sep2 = new QLabel("|");
    stl->addWidget(sep2);
    m_byteSizeLabel = new QLabel("0 KB");
    stl->addWidget(m_byteSizeLabel);
    rpl->addWidget(stBar);

    // Empty state overlay
    m_emptyState = new QLabel("选择或新建一篇笔记");
    m_emptyState->setObjectName("emptyStateLabel");
    m_emptyState->setAlignment(Qt::AlignCenter);
    m_emptyState->hide();
    rpl->addWidget(m_emptyState);

    m_settingsPanel = new SettingsPanel;
    m_settingsPanel->hide();
    connect(m_settingsPanel, &SettingsPanel::closeRequested, this, &MainWindow::toggleSettings);
    connect(m_settingsPanel, &SettingsPanel::configChanged, this, &MainWindow::onSettingsChanged);

    root->addWidget(rp);
    // Border between main area and settings panel
    auto *settingsBorder = new QFrame; settingsBorder->setFrameShape(QFrame::VLine);
    settingsBorder->setObjectName("settingsBorder");
    settingsBorder->setFixedWidth(4);
    root->addWidget(settingsBorder);
    root->addWidget(m_settingsPanel);
    mv->addWidget(body, 1);  // stretch=1 so body fills remaining space
}


// ── 拖拽 ──
void MainWindow::mousePressEvent(QMouseEvent *e) {
    if (e->button()==Qt::LeftButton && e->pos().y()<44) {
        if (auto *wh = windowHandle()) { wh->startSystemMove(); return; }
    }
    QMainWindow::mousePressEvent(e);
}
void MainWindow::mouseMoveEvent(QMouseEvent *e) {
    QMainWindow::mouseMoveEvent(e);
}
void MainWindow::mouseDoubleClickEvent(QMouseEvent *e) {
    if (e->pos().y()<44) { isMaximized()?showNormal():showMaximized(); }
    QMainWindow::mouseDoubleClickEvent(e);
}

// ── 窗口状态变化 ──
void MainWindow::changeEvent(QEvent *e) {
    if (e->type() == QEvent::WindowStateChange && m_maxBtn) {
        m_maxBtn->setIcon(FloralTheme::icon(
            isMaximized() ? ":/icons/window-restore.svg" : ":/icons/window-maximize.svg"));
    }
    QMainWindow::changeEvent(e);
}

// ── 窗口大小变化 ──
void MainWindow::resizeEvent(QResizeEvent *e) {
    // Prevent preview from being hidden when window gets too narrow
    int minSplitter = m_editorContainer->minimumWidth() + m_previewContainer->minimumWidth() + 4;
    if (width() < minSplitter && m_currentViewMode == "split") {
        m_currentViewMode = "edit";
        m_viewModeGroup->setCurrentValue("edit");
        m_editorContainer->setVisible(true);
        m_previewContainer->setVisible(false);
    }
    QMainWindow::resizeEvent(e);
}
void MainWindow::initialize(NoteStore *store, ConfigManager *configMgr) {
    m_store = store; m_configMgr = configMgr;
    m_config = configMgr->load();
    applyConfig(m_config);
    rebuildNoteList();
}

// ── 笔记列表重建 ──
QWidget *MainWindow::makeNoteItem(const NoteMetadata &meta, bool inCategory) {
    auto *w = new QPushButton;
    w->setObjectName("noteItem");
    w->setFlat(true);
    w->setCursor(Qt::PointingHandCursor);
    w->setProperty("noteId", meta.id);
    if (inCategory) w->setProperty("noteInCategory", true);
    w->setFixedHeight(72);
    auto *outer = new QVBoxLayout(w);
    outer->setContentsMargins(20,8,0,8);
    outer->setSpacing(4);

    // Indicator bar — 4px wide rounded rect, 8px from card edges
    auto *indicator = new QWidget(w);
    indicator->setObjectName("noteIndicator");
    indicator->setFixedSize(4, 40);
    indicator->move(6, 16);
    indicator->setProperty("indicatorOn", false);

    // Row 1: title only
    auto *row1 = new QHBoxLayout;
    row1->setSpacing(8);
    auto *title = new QLabel(meta.title.isEmpty()?"未命名":meta.title);
    title->setObjectName("noteTitle");
    title->setWordWrap(false);
    title->setTextFormat(Qt::PlainText);
    title->setProperty("fullText", meta.title.isEmpty()?"未命名":meta.title);
    title->installEventFilter(this);
    row1->addWidget(title, 1);
    outer->addLayout(row1);

    // Row 2: preview — single line
    auto *preview = new QLabel(meta.preview.isEmpty()?"空白笔记":meta.preview);
    preview->setObjectName("notePreview");
    preview->setWordWrap(false);
    preview->setTextFormat(Qt::PlainText);
    preview->setProperty("fullText", meta.preview.isEmpty()?"空白笔记":meta.preview);
    preview->setMaximumHeight(16);
    outer->addWidget(preview);

    // Larger gap to row 3
    outer->addSpacing(6);

    // Row 3: date + time + word count
    auto *row3 = new QHBoxLayout;
    row3->setSpacing(4);
    auto *createdDate = new QLabel(meta.createdAt.toString("MM-dd"));
    createdDate->setObjectName("noteDate");
    row3->addWidget(createdDate);
    auto *createdTime = new QLabel(meta.createdAt.toString("hh:mm"));
    createdTime->setObjectName("noteTime");
    row3->addWidget(createdTime);
    auto *createdLabel = new QLabel("创建");
    createdLabel->setObjectName("noteDate");
    row3->addWidget(createdLabel);
    auto *dot1 = new QLabel("·");
    dot1->setObjectName("noteDot");
    row3->addWidget(dot1);
    auto *modDate = new QLabel(meta.updatedAt.toString("MM-dd"));
    modDate->setObjectName("noteDate");
    row3->addWidget(modDate);
    auto *modTime = new QLabel(meta.updatedAt.toString("hh:mm"));
    modTime->setObjectName("noteTime");
    row3->addWidget(modTime);
    auto *modLabel = new QLabel("修改");
    modLabel->setObjectName("noteDate");
    row3->addWidget(modLabel);
    auto *dot2 = new QLabel("·");
    dot2->setObjectName("noteDot");
    row3->addWidget(dot2);
    auto *wc = new QLabel(QString("%1 字").arg(meta.wordCount));
    wc->setObjectName("noteWordCount");
    row3->addWidget(wc);
    row3->addStretch();
    outer->addLayout(row3);

    connect(w, &QPushButton::clicked, this, [this, id=meta.id](){ onNoteClicked(id); });
    w->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(w, &QWidget::customContextMenuRequested, this, [this, id=meta.id](const QPoint &) {
        showMoveCategoryMenu(id);
    });
    w->installEventFilter(this);

    return w;
}

QWidget *MainWindow::makeCategoryGroup(const QString &cat, const QVector<NoteMetadata> &notes) {
    auto *grp = new QWidget;
    auto *gl = new QVBoxLayout(grp);
    gl->setContentsMargins(6,0,6,4); // Tauri: px-2 mb-1.5 → tighter
    gl->setSpacing(0);

    bool isUncat = cat.isEmpty();
    auto *hdr = new CategoryHeader(cat, isUncat, nullptr);
    hdr->setCount(notes.size());

    m_catHeaders[cat] = hdr;
    connect(hdr, &CategoryHeader::toggled, this, &MainWindow::toggleCategory);
    connect(hdr, &CategoryHeader::renameRequested, this, &MainWindow::onRenameCategory);
    connect(hdr, &CategoryHeader::deleteRequested, this, &MainWindow::onDeleteCategory);
    gl->addWidget(hdr);

    auto *bodyWidget = new QWidget;
    bodyWidget->setObjectName("catBody_"+cat);
    bodyWidget->setProperty("categoryBody", true);
    auto *bl = new QVBoxLayout(bodyWidget);
    bl->setContentsMargins(2,2,2,2);
    bl->setSpacing(8);

    for (auto &n : notes) {
        auto *item = makeNoteItem(n, !isUncat);
        item->setProperty("category", cat);
        m_noteItems[n.id] = item;
        bl->addWidget(item);
    }
    gl->addWidget(bodyWidget);
    return grp;
}

void MainWindow::rebuildNoteList() {
    if (!m_store) return;
    m_noteItems.clear();
    m_catHeaders.clear();

    // Clear sidebar content
    QLayoutItem *child;
    while ((child = m_sidebarLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }

    auto notes = m_store->listNotes();

    // Filter
    if (!m_searchFilter.isEmpty()) {
        notes.erase(std::remove_if(notes.begin(), notes.end(), [this](const NoteMetadata &m) {
            return !m.title.contains(m_searchFilter, Qt::CaseInsensitive) &&
                   !m.preview.contains(m_searchFilter, Qt::CaseInsensitive);
        }), notes.end());
    }

    // Update note count label
    auto *ncl = findChild<QLabel*>("sidebarNoteCount");
    if (ncl) ncl->setText(QString("%1 篇笔记").arg(notes.size()));

    // Group by category
    QMap<QString, QVector<NoteMetadata>> groups;
    for (auto &n : notes) groups[n.category].append(n);

    for (auto it = groups.begin(); it != groups.end(); ++it) {
        if (it.key().isEmpty()) {
            // Uncategorized notes: list directly without a category wrapper
            for (auto &n : it.value()) {
                auto *item = makeNoteItem(n, false);
                m_noteItems[n.id] = item;
                m_sidebarLayout->addWidget(item);
            }
        } else {
            auto *grp = makeCategoryGroup(it.key(), it.value());
            m_sidebarLayout->addWidget(grp);
        }
    }
    m_sidebarLayout->addStretch();

    // Update selection highlight
    for (auto it = m_noteItems.begin(); it != m_noteItems.end(); ++it) {
        bool sel = (it.key() == m_selectedNoteId);
        it.value()->setProperty("noteSelected", sel);
        if (auto *ind = it.value()->findChild<QWidget*>("noteIndicator")) {
            ind->setProperty("indicatorOn", sel);
            ind->style()->unpolish(ind);
            ind->style()->polish(ind);
        }
        it.value()->style()->unpolish(it.value());
        it.value()->style()->polish(it.value());
        it.value()->update();
    }
}

// ── 交互 ──
void MainWindow::onNoteClicked(const QString &id) {
    if (id == m_selectedNoteId && !m_dirty) return;
    if (m_dirty && !m_selectedNoteId.isEmpty()) onSaveNote();
    m_selectedNoteId = id;
    loadNote(id);
    // Update title bar subtitle
    if (m_subTitleLabel) {
        m_subTitleLabel->setProperty("fullText", m_titleEdit->text().isEmpty()?"无标题笔记":m_titleEdit->text());
        updateSubTitleElide();
    }
    // Highlight
    for (auto it = m_noteItems.begin(); it != m_noteItems.end(); ++it) {
        bool s = it.key() == id;
        it.value()->setProperty("noteSelected", s);
        // Update indicator bar
        if (auto *ind = it.value()->findChild<QWidget*>("noteIndicator")) {
            ind->setProperty("indicatorOn", s);
            ind->style()->unpolish(ind);
            ind->style()->polish(ind);
        }
        it.value()->style()->unpolish(it.value());
        it.value()->style()->polish(it.value());
        it.value()->update();
    }
    // Hide empty state
    if (m_emptyState) m_emptyState->hide();
}

void MainWindow::loadNote(const QString &id) {
    if (!m_store) return;
    try {
        auto note = m_store->readNote(id);
        m_editor->blockSignals(true);
        m_editor->setPlainText(note.content);
        m_titleEdit->blockSignals(true); m_titleEdit->setText(note.title); m_titleEdit->setCursorPosition(0); m_titleEdit->blockSignals(false);
        m_editor->blockSignals(false);
        m_preview->setMarkdownContent(note.content, m_config.fontSize, m_config.markdownTheme);
        if (m_dateLabel) m_dateLabel->setText(note.updatedAt.toString("M-d hh:mm"));
        if (m_createdLabel) m_createdLabel->setText(note.createdAt.toString("M-d hh:mm"));
        m_dirty = false;
        updateStatusBar();
    } catch(...) {}
}

void MainWindow::onNewNote() {
    if (!m_store) return;
    SaveNoteRequest r; r.title="新笔记"; r.content="";
    auto n = m_store->createNote(r);
    rebuildNoteList();
    onNoteClicked(n.id);
}

void MainWindow::onSaveNote() {
    if (!m_store || m_selectedNoteId.isEmpty()) return;
    m_saveState = Saving;
    updateStatusBar();
    SaveNoteRequest r;
    r.title = extractTitle(m_editor->toPlainText());
    r.content = m_editor->toPlainText();
    try {
        m_store->updateNote(m_selectedNoteId, r);
        m_dirty = false; m_saveState = Saved;
        m_preview->setMarkdownContent(r.content, m_config.fontSize, m_config.markdownTheme);
        // Refresh modification time in editor meta bar
        auto saved = m_store->readNote(m_selectedNoteId);
        if (m_dateLabel) m_dateLabel->setText(saved.updatedAt.toString("M-d hh:mm"));
        rebuildNoteList();
    } catch(...) { m_saveState = Error; }
    updateStatusBar();
    QTimer::singleShot(2000, this, [this](){ if(m_saveState==Saved) m_saveState=Idle; updateStatusBar(); });
}

void MainWindow::onDeleteNote() {
    if (!m_store || m_selectedNoteId.isEmpty()) return;
    if (QMessageBox::question(this,"删除","确认删除？")==QMessageBox::Yes) {
        m_store->deleteNote(m_selectedNoteId);
        m_selectedNoteId.clear();
        m_editor->clear();
        m_titleEdit->clear();
        if (m_dateLabel) m_dateLabel->setText("--");
        if (m_createdLabel) m_createdLabel->setText("--");
        m_preview->setMarkdownContent("", m_config.fontSize, m_config.markdownTheme);
        rebuildNoteList();
        updateStatusBar();
    }
}

void MainWindow::onContentChanged() {
    m_dirty = true; m_saveState = Dirty;
    // Update title bar subtitle in real time
    if (m_subTitleLabel && m_titleEdit) {
        QString full = m_titleEdit->text().trimmed().isEmpty()?"无标题笔记":m_titleEdit->text().trimmed();
        m_subTitleLabel->setProperty("fullText", full);
        updateSubTitleElide();
    }
    updateStatusBar();
    if (m_config.noteAutoSave) {
        m_autoSaveTimer = new QTimer(this);
        m_autoSaveTimer->setSingleShot(true);
        connect(m_autoSaveTimer, &QTimer::timeout, this, [this](){
            if (m_dirty && !m_selectedNoteId.isEmpty()) onSaveNote();
        });
        m_autoSaveTimer->start(2000);
    }
}

void MainWindow::onViewModeChanged(const QString &mode) {
    m_currentViewMode = mode;
    m_editorContainer->setVisible(mode!="preview");
    m_previewContainer->setVisible(mode!="edit");
    if (mode=="split") {
        int minTotal = m_editorContainer->minimumWidth() + m_previewContainer->minimumWidth();
        int avail = width() - minTotal;
        m_splitter->setSizes({m_editorContainer->minimumWidth() + avail/2, m_previewContainer->minimumWidth() + avail/2});
        m_editorContainer->show(); m_previewContainer->show();
        // Sync preview to editor scroll position when entering split mode
        QTimer::singleShot(0, this, [this](){
            if (m_preview->isVisible()) {
                m_preview->verticalScrollBar()->blockSignals(true);
                m_preview->verticalScrollBar()->setValue(m_editor->verticalScrollBar()->value());
                m_preview->verticalScrollBar()->blockSignals(false);
            }
        });
    }
}

void MainWindow::toggleSettings() {
    if (!m_settingsPanel) return;
    bool opening = !m_settingsOpen;
    m_settingsOpen = opening;

    if (m_settingsAnim) {
        m_settingsAnim->stop();
        delete m_settingsAnim;
    }

    if (opening) {
        m_settingsPanel->loadConfig(m_config);
        m_settingsPanel->setMinimumWidth(0);
        m_settingsPanel->setMaximumWidth(0);  // animation from 0 → 360
        // animation owns maximumWidth, don't pre-set QWIDGETSIZE_MAX
        m_settingsPanel->show();
    } else {
        m_settingsPanel->setMinimumWidth(0);
        m_settingsPanel->setMaximumWidth(m_settingsPanel->width());
    }

    m_settingsAnim = new QPropertyAnimation(m_settingsPanel, "maximumWidth", this);
    m_settingsAnim->setDuration(400);
    m_settingsAnim->setEasingCurve(QEasingCurve::InOutCubic);

    if (opening) {
        m_settingsAnim->setStartValue(0);
        m_settingsAnim->setEndValue(360);
    } else {
        m_settingsAnim->setStartValue(m_settingsPanel->width());
        m_settingsAnim->setEndValue(0);
    }

    connect(m_settingsAnim, &QPropertyAnimation::finished, this, [this, opening](){
        if (!opening) {
            m_settingsPanel->hide();
            m_settingsPanel->setFixedWidth(360);
        } else {
            m_settingsPanel->setFixedWidth(360);
        }
    });

    m_settingsAnim->start(QAbstractAnimation::DeleteWhenStopped);
    m_settingsAnim = nullptr;
}

void MainWindow::onSettingsChanged(const AppConfig &config) {
    m_config=config; if(m_configMgr) m_configMgr->save(config);
    applyConfig(config);
}

void MainWindow::applyConfig(const AppConfig &config) {
    int editorScroll = m_editor->verticalScrollBar()->value();
    int previewScroll = m_preview ? m_preview->verticalScrollBar()->value() : 0;

    if(config.nightMode=="light") FloralTheme::instance().apply(FloralThemeOption::Light);
    else if(config.nightMode=="dark") FloralTheme::instance().apply(FloralThemeOption::Dark);
    else FloralTheme::instance().apply(FloralThemeOption::System);
    if(m_editor){ QFont f=m_editor->font(); f.setPixelSize(config.fontSize); m_editor->setFont(f); }
    m_viewModeGroup->blockSignals(true); m_viewModeGroup->setCurrentValue(config.defaultViewMode); m_viewModeGroup->blockSignals(false);
    // Refresh markdown preview if theme changed
    if (m_preview && !m_selectedNoteId.isEmpty()) {
        QString content = m_store->readNote(m_selectedNoteId).content;
        m_preview->setMarkdownContent(content, config.fontSize, config.markdownTheme);
    }
    // Restore scroll positions after theme/content reload
    m_editor->verticalScrollBar()->setValue(editorScroll);
    if (m_preview) m_preview->verticalScrollBar()->setValue(previewScroll);
    // Refresh status label & note item colors after theme switch
    updateStatusBar();
    for (auto it = m_noteItems.begin(); it != m_noteItems.end(); ++it) {
        if (auto *ind = it.value()->findChild<QWidget*>("noteIndicator")) {
            ind->style()->unpolish(ind);
            ind->style()->polish(ind);
        }
        it.value()->style()->unpolish(it.value());
        it.value()->style()->polish(it.value());
        it.value()->update();
    }
}

void MainWindow::updateSubTitleElide() {
    if (!m_subTitleLabel || !m_viewModeGroup || !m_sidebarToggleBtn) return;
    QString full = m_subTitleLabel->property("fullText").toString();
    if (full.isEmpty()) return;
    int rightEdge = m_viewModeGroup->x() - m_sidebarToggleBtn->width() - 8;
    int maxW = rightEdge - m_subTitleLabel->x();
    if (maxW < 40) maxW = 40;
    QFontMetrics fm(m_subTitleLabel->font());
    m_subTitleLabel->setText(fm.elidedText(full, Qt::ElideRight, maxW));
}

void MainWindow::updateStatusBar() {
    static const char *labels[] = {"未保存","未保存","保存中","已保存","保存失败"};
    if (!m_editor) return;
    QString text = m_editor->toPlainText();
    int lines = text.isEmpty()?0:text.count('\n')+1;
    m_lineCountLabel->setText(QString("Ln %1").arg(lines));
    int bytes = text.toUtf8().size();
    m_byteSizeLabel->setText(bytes<1024?QString("%1 B").arg(bytes):QString("%1 KB").arg(bytes/1024.0,0,'f',1));
    // Update meta row
    if (m_charCountLabel) {
        int chars = text.length();
        m_charCountLabel->setText(QString("%1 字").arg(chars));
    }
    // Update save state label
    if (m_metaStatusLabel) {
        m_metaStatusLabel->setText(labels[m_saveState]);
        auto&p = FloralTheme::instance().current();
        QString c;
        switch (m_saveState) {
        case Error: c = "#ef4444"; break;
        case Dirty: {
            QColor amber(0xf9,0x9c,0x00,179);
            c = amber.name(QColor::HexArgb); break;
        }
        default: {
            QColor bm(p.bamboo.red(),p.bamboo.green(),p.bamboo.blue(),153);
            c = bm.name(QColor::HexArgb); break;
        }
        }
        m_metaStatusLabel->setStyleSheet(
            QString("#saveStateLabel{color:%1;}").arg(c));
    }
}

// ── 分类操作 ──
void MainWindow::toggleCategory(const QString &name) {
    auto *body = m_sidebarContent->findChild<QWidget*>("catBody_"+name);
    auto *hdr = m_catHeaders.value(name);
    if (body && hdr) {
        bool visible = !body->isVisible();
        body->setVisible(visible);
        hdr->setExpanded(visible);
        hdr->style()->unpolish(hdr);
        hdr->style()->polish(hdr);
    }
}
void MainWindow::onNewCategory() {
    bool ok; QString name = QInputDialog::getText(this,"新建分类","分类名:",QLineEdit::Normal,"",&ok);
    if (ok && !name.isEmpty()) { m_store->createCategory(name); rebuildNoteList(); }
}
void MainWindow::onRenameCategory(const QString &oldName) {
    bool ok; QString n = QInputDialog::getText(this,"重命名分类","新名称:",QLineEdit::Normal,oldName,&ok);
    if (ok && !n.isEmpty()) { m_store->renameCategory(oldName,n); rebuildNoteList(); }
}
void MainWindow::onDeleteCategory(const QString &name) {
    if (QMessageBox::question(this,"删除分类",QString("确认删除「%1」？").arg(name))==QMessageBox::Yes)
    { m_store->deleteCategory(name); rebuildNoteList(); }
}

// ── 格式 ──
void MainWindow::onFormatBtn(const QString &action) { applyFormat(action); }
void MainWindow::applyFormat(const QString &action) {
    QTextCursor cur = m_editor->textCursor();
    QString sel = cur.selectedText();
    int s = cur.selectionStart(), e = cur.selectionEnd();

    if (action=="bold") { cur.insertText("**"+(sel.isEmpty()?"粗体文本":sel)+"**"); }
    else if (action=="italic") { cur.insertText("*"+(sel.isEmpty()?"斜体文本":sel)+"*"); }
    else if (action=="heading") {
        cur.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
        cur.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        QString line = cur.selectedText();
        cur.removeSelectedText();
        cur.insertText("## " + line.trimmed());
    }
    else if (action=="hr") { cur.insertText("\n---\n"); }
    else if (action=="ul") {
        if (sel.contains('\n')) {
            QStringList lines = sel.split('\n');
            QString result;
            for (auto &l : lines) result += "- " + l + "\n";
            cur.insertText(result);
        } else cur.insertText("- "+(sel.isEmpty()?"列表项":sel));
    }
    else if (action=="ol") {
        if (sel.contains('\n')) {
            QStringList lines = sel.split('\n');
            QString result;
            for (int i=0;i<lines.size();++i) result += QString::number(i+1)+". "+lines[i]+"\n";
            cur.insertText(result);
        } else cur.insertText("1. "+(sel.isEmpty()?"列表项":sel));
    }
    else if (action=="code") {
        if (sel.contains('\n')) cur.insertText("```\n"+sel+"\n```");
        else cur.insertText("`"+(sel.isEmpty()?"代码":sel)+"`");
    }
    else if (action=="quote") {
        if (sel.contains('\n')) {
            QStringList lines = sel.split('\n');
            QString result;
            for (auto &l : lines) result += "> " + l + "\n";
            cur.insertText(result);
        } else cur.insertText("> "+(sel.isEmpty()?"引用":sel));
    }
    m_editor->setTextCursor(cur);
    m_editor->setFocus();
}

// ── 搜索 ──
void MainWindow::onSearchChanged(const QString &text) { m_searchFilter = text; rebuildNoteList(); }

// ── 移动分类 ──
void MainWindow::showMoveCategoryMenu(const QString &noteId) {
    QMenu menu;
    menu.addAction("导出 Markdown", this, [this,noteId](){
        QString p = QFileDialog::getSaveFileName(this,"导出",noteId+".md","Markdown (*.md)");
        if (!p.isEmpty()) m_store->exportMarkdownFile(noteId, p);
    });
    menu.addSeparator();
    QAction *delAction = menu.addAction("删除笔记");
    delAction->setData("danger");
    connect(delAction, &QAction::triggered, this, [this,noteId](){
        if (QMessageBox::question(this,"删除","确认删除？")==QMessageBox::Yes)
        { m_store->deleteNote(noteId); m_selectedNoteId.clear(); m_editor->clear(); rebuildNoteList(); updateStatusBar(); }
    });
    menu.exec(QCursor::pos());
}
void MainWindow::moveNoteToCategory(const QString &noteId, const QString &cat) {
    m_store->moveNoteToCategory(noteId, cat);
    rebuildNoteList();
}
void MainWindow::startRenameNote(const QString &noteId) {
    // TODO: inline rename
}

QString MainWindow::extractTitle(const QString &content) const {
    QString first = content.section('\n',0,0).trimmed();
    if (first.startsWith('#')) first = first.mid(first.indexOf(' ')+1).trimmed();
    return first.isEmpty()?"笔记":first.left(50);
}

// ── 侧栏折叠 ──
void MainWindow::toggleSidebar() {
    if (!m_sidebarWidget) return;
    m_sidebarCollapsed = !m_sidebarCollapsed;

    if (m_sidebarAnim) {
        m_sidebarAnim->stop();
        delete m_sidebarAnim;
    }

    // Release fixed width; let the animation own maximumWidth
    m_sidebarWidget->setMinimumWidth(0);
    m_sidebarWidget->setMaximumWidth(m_sidebarCollapsed ? m_sidebarWidth : 0);

    m_sidebarAnim = new QPropertyAnimation(m_sidebarWidget, "maximumWidth", this);
    m_sidebarAnim->setDuration(450);
    m_sidebarAnim->setEasingCurve(QEasingCurve::InOutCubic);

    if (m_sidebarCollapsed) {
        m_sidebarAnim->setStartValue(m_sidebarWidth);
        m_sidebarAnim->setEndValue(0);
        m_sidebarToggleBtn->setToolTip("展开侧栏");
        if (m_sidebarHandle) m_sidebarHandle->setCursor(Qt::ArrowCursor);
    } else {
        m_sidebarAnim->setStartValue(0);
        m_sidebarAnim->setEndValue(m_sidebarWidth);
        m_sidebarToggleBtn->setToolTip("收起侧栏");
        if (m_sidebarHandle) m_sidebarHandle->setCursor(Qt::SplitHCursor);
    }

    connect(m_sidebarAnim, &QPropertyAnimation::finished, this, [this](){
        if (!m_sidebarCollapsed) {
            m_sidebarWidget->setFixedWidth(m_sidebarWidth);
        } else {
            m_sidebarWidget->setFixedWidth(0);
            m_sidebarWidget->setMaximumWidth(0);
        }
    });

    m_sidebarAnim->start(QAbstractAnimation::DeleteWhenStopped);
    m_sidebarAnim = nullptr;
}

// ── 钉为磁贴 ──
void MainWindow::onPinToTile() {
    if (m_selectedNoteId.isEmpty()) return;
    if (m_dirty) onSaveNote();
    auto *pad = new NotePadWindow;
    pad->initialize(m_store, m_configMgr, m_selectedNoteId, "tile");
    pad->show();
}

// ── 撤销 ──
void MainWindow::onUndo() {
    if (m_editor && !m_selectedNoteId.isEmpty()) {
        m_editor->undo();
        m_dirty = true;
        updateStatusBar();
    }
}

// ── 删除确认 ── (按钮已移除，保留桩)
void MainWindow::onDeleteClicked() { }

// ── 事件过滤（内联分类输入 Escape / 失焦） ──
bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    // ── 标题栏 viewMode 居中 + sidebarBtn 定位 + subTitle 省略 ──
    if (obj->objectName() == "titleBar" && event->type() == QEvent::Resize && m_viewModeGroup) {
        auto *bar = static_cast<QWidget*>(obj);
        int cx = (bar->width() - m_viewModeGroup->width()) / 2;
        int cy = (bar->height() - m_viewModeGroup->height()) / 2;
        m_viewModeGroup->move(cx, cy);
        if (m_sidebarToggleBtn) {
            int sbx = cx - m_sidebarToggleBtn->width() - 12;
            int sby = (bar->height() - m_sidebarToggleBtn->height()) / 2;
            m_sidebarToggleBtn->move(sbx, sby);
            m_sidebarToggleBtn->raise();
            m_sidebarToggleBtn->show();
        }
        // Re-elide subTitle so it doesn't overlap sidebarBtn / viewModeGroup
        updateSubTitleElide();
        return false;
    }

    // ── 侧边栏拖拽手柄 ──
    if (obj == m_sidebarHandle && !m_sidebarCollapsed) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto *me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                m_resizingSidebar = true;
                m_resizeStartX = me->globalPosition().x();
                m_sidebarHandle->grabMouse();
                return true;
            }
        }
        if (event->type() == QEvent::MouseMove && m_resizingSidebar) {
            auto *me = static_cast<QMouseEvent*>(event);
            int delta = me->globalPosition().x() - m_resizeStartX;
            m_resizeStartX = me->globalPosition().x();
            int newWidth = qBound(180, m_sidebarWidget->width() + delta, 500);
            m_sidebarWidth = newWidth;
            m_sidebarWidget->setFixedWidth(newWidth);
            return true;
        }
        if (event->type() == QEvent::MouseButtonRelease && m_resizingSidebar) {
            m_resizingSidebar = false;
            m_sidebarHandle->releaseMouse();
            return true;
        }
    }

    // ── 标题 elide：noteTitle resize 时重新计算省略号 ──
    if (obj->objectName() == "noteTitle" && event->type() == QEvent::Resize) {
        auto *label = static_cast<QLabel*>(obj);
        QString full = label->property("fullText").toString();
        if (!full.isEmpty()) {
            QFontMetrics fm(label->font());
            QString elided = fm.elidedText(full, Qt::ElideRight, label->width());
            label->setText(elided);
        }
        return false;
    }

    // ── Marquee：noteItem hover 2s 后正文滚动 ──
    if (obj->objectName() == "noteItem") {
        auto *btn = static_cast<QPushButton*>(obj);
        auto *preview = btn->findChild<QLabel*>("notePreview");
        if (!preview) return QMainWindow::eventFilter(obj, event);

        if (event->type() == QEvent::Enter) {
            // Only scroll if text exceeds label width
            QString full = preview->property("fullText").toString();
            if (preview->fontMetrics().horizontalAdvance(full) <= preview->width())
                return false;
            auto *timer = new QTimer(btn);
            timer->setObjectName("marqueeTimer");
            timer->setSingleShot(true);
            connect(timer, &QTimer::timeout, btn, [btn](){
                auto *preview = btn->findChild<QLabel*>("notePreview");
                if (!preview) return;
                QString full = preview->property("fullText").toString();
                if (full.isEmpty()) return;
                preview->setProperty("marqueeRunning", true);
                auto *tick = new QTimer(btn);
                tick->setObjectName("marqueeTick");
                tick->setInterval(120);
                preview->setProperty("marqueeOffset", 0);
                connect(tick, &QTimer::timeout, btn, [btn](){
                    auto *preview = btn->findChild<QLabel*>("notePreview");
                    if (!preview) return;
                    QString full = preview->property("fullText").toString();
                    int off = preview->property("marqueeOffset").toInt() + 1;
                    if (off >= full.length()) off = 0;
                    preview->setProperty("marqueeOffset", off);
                    preview->setText(full.mid(off) + "   " + full.left(off));
                });
                tick->start();
            });
            timer->start(2000);
            return false;
        }
        if (event->type() == QEvent::Leave) {
            if (auto *t = btn->findChild<QTimer*>("marqueeTimer")) { t->stop(); delete t; }
            if (auto *t = btn->findChild<QTimer*>("marqueeTick")) { t->stop(); delete t; }
            preview->setProperty("marqueeRunning", false);
            preview->setText(preview->property("fullText").toString());
            return false;
        }
    }

    // ── QSplitter handle drag: clamp so neither canvas goes below 200px ──
    if (obj == m_splitter && event->type() == QEvent::MouseButtonRelease) {
    }

    return QMainWindow::eventFilter(obj, event);
}
