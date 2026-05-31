#include "theme.h"
#include <QPalette>
#include <QWidget>
#include <QFont>
#include <QApplication>
#include <QIcon>
#include <QPixmap>
#include <QFile>
#include <QSvgRenderer>
#include <QPainter>

// Helper: create #AARRGGBB hex with alpha
static QString ahex(const QColor &c, int alpha) {
    QColor ac(c.red(), c.green(), c.blue(), alpha);
    return ac.name(QColor::HexArgb);
}

FloralPalette FloralPalette::light() {
    return {
        QColor("#f6f3ec"), QColor("#f0ebe0"), QColor("#e8e1d3"),
        QColor("#1a1a18"), QColor("#3d3d38"), QColor("#8a8a80"),
        QColor("#b8b8ae"), QColor("#2d5a3d"), QColor("#3a7a52"),
        QColor("#e8f0eb"), QColor("#d4e8da"), QColor("#6b6b62"),
        QColor("#ffffff"),
        QColor(26,26,24,15),  QColor(26,26,24,30),  QColor("#fef2f2")
    };
}
FloralPalette FloralPalette::dark() {
    return {
        QColor("#222120"), QColor("#2c2a27"), QColor("#3c3935"),
        QColor("#e5e1da"), QColor("#b5b1a8"), QColor("#928f87"),
        QColor("#706d67"), QColor("#4faa70"), QColor("#5fc085"),
        QColor("#1c2e22"), QColor("#243a2c"), QColor("#8a8880"),
        QColor("#1a1917"),
        QColor(0,0,0,76),    QColor(0,0,0,127),     QColor(220,38,38,38)
    };
}

FloralTheme &FloralTheme::instance(){static FloralTheme i;return i;}
FloralTheme::FloralTheme(){m_palette=FloralPalette::light();}

void FloralTheme::apply(FloralThemeOption o){
    m_option=o;
    if(o==FloralThemeOption::Light) m_palette=FloralPalette::light();
    else if(o==FloralThemeOption::Dark) m_palette=FloralPalette::dark();
    else{
        QPalette s=QApplication::palette();
        m_palette=s.color(QPalette::Window).lightness()<128
            ?FloralPalette::dark():FloralPalette::light();
    }
    applyPalette();
    emit themeChanged();
}
bool FloralTheme::isDark()const{return m_palette.paper.lightness()<128;}

void FloralTheme::applyPalette(){
    auto&p=m_palette;

    // -- Shortcuts --
    QString P  = p.paper.name();          //  paper
    QString PW = p.paperWarm.name();      //  paperWarm
    QString PD = p.paperDeep.name();      //  paperDeep
    QString I  = p.ink.name();            //  ink
    QString IS = p.inkSoft.name();        //  inkSoft
    QString IF = p.inkFaint.name();       //  inkFaint
    QString IG = p.inkGhost.name();       //  inkGhost
    QString B  = p.bamboo.name();         //  bamboo
    QString BL = p.bambooLight.name();    //  bambooLight
    QString BM = p.bambooMist.name();     //  bambooMist
    QString BG = p.bambooGlow.name();     //  bambooGlow
    QString CL = p.cloud.name();          //  cloud
    QString DB = p.dangerBg.name();       //  dangerBg

    // -- Alpha variants (matching Tauri CSS transparency) --
    QString PW60 = ahex(p.paperWarm, 153);
    QString PW70 = ahex(p.paperWarm, 179);
    QString PW80 = ahex(p.paperWarm, 204);
    QString PW45 = ahex(p.paperWarm, 115);
    QString PD30 = ahex(p.paperDeep, 76);
    QString PD20 = ahex(p.paperDeep, 51);
    QString PD15 = ahex(p.paperDeep, 38);
    QString PD25 = ahex(p.paperDeep, 64);
    QString PD40 = ahex(p.paperDeep, 102);
    QString PD50 = ahex(p.paperDeep, 128);
    QString BM90 = ahex(p.bambooMist, 230);
    QString BM70 = ahex(p.bambooMist, 179);
    QString BM80 = ahex(p.bambooMist, 204);
    QString BM60 = ahex(p.bambooMist, 153);
    QString BM50 = ahex(p.bambooMist, 128);
    QString BH8  = ahex(p.bamboo, 20);
    QString BH10 = ahex(p.bamboo, 26);
    QString BH15 = ahex(p.bamboo, 38);
    QString BH30 = ahex(p.bamboo, 76);
    QString BH40 = ahex(p.bamboo, 102);
    QString BH50 = ahex(p.bamboo, 128);
    QString BH60 = ahex(p.bamboo, 153);
    QString BH70 = ahex(p.bamboo, 179);
    QString BH80 = ahex(p.bamboo, 204);
    QString BH03 = ahex(p.bamboo, 8);
    QString PP40 = ahex(p.paper, 102);
    QString PP20 = ahex(p.paper, 51);
    QString PP30 = ahex(p.paper, 76);
    QString PP60 = ahex(p.paper, 153);
    QString CL92 = ahex(p.cloud, 235);
    QString CL95 = ahex(p.cloud, 242);
    QString IG60 = ahex(p.inkGhost, 153);
    QString IG50 = ahex(p.inkGhost, 128);
    QString IG40 = ahex(p.inkGhost, 102);

    // Icon colors: lighter in dark mode so SVG currentColor stays visible
    QString IC  = isDark() ? IS : IG;   // icon default
    QString ICH = isDark() ? I  : IF;   // icon hover / active

    // Sidebar button hover — slightly lighter than BL, looks good on both light & dark
    QString NBH = isDark() ? QString("#75d098") : QString("#4d8c63");
    // Import button hover — lighter than IF in light mode
    QString IBH = isDark() ? IS : IG;

    QString q;

    // ═══ Global defaults ═══
    q += "*{font-family:'Noto Sans SC','Source Han Sans SC',sans-serif;}"
         "QMainWindow{background:"+P+";}";

    // ═══ Scrollbar ═══
    q += "QScrollBar:vertical{width:8px;background:transparent;margin:0;border:none;}"
         "QScrollBar::handle:vertical{background:"+IG+";border-radius:4px;min-height:40px;}"
         "QScrollBar::handle:vertical:hover{background:"+IF+";}"
         "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0px;border:none;background:transparent;}"
         "QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical{background:transparent;border:none;}"
         "QScrollBar:horizontal{height:8px;background:transparent;border:none;}"
         "QScrollBar::handle:horizontal{background:"+IG+";border-radius:4px;min-width:40px;}"
         "QScrollBar::handle:horizontal:hover{background:"+IF+";}"
         "QScrollBar::add-line:horizontal,QScrollBar::sub-line:horizontal{width:0px;border:none;background:transparent;}"
         "QScrollBar::add-page:horizontal,QScrollBar::sub-page:horizontal{background:transparent;border:none;}";

    // ═══ Text inputs ═══
    q += "QLineEdit,QTextEdit,QPlainTextEdit{background:transparent;border:none;outline:none;color:"+IS+";}"
         "QLineEdit{padding:4px 10px;font-size:12px;}"
         "QTextEdit,QPlainTextEdit{padding:8px 12px;"
         "  font-size:14px;"
         "  font-family:'JetBrains Mono','Fira Code',monospace;"
         "  selection-background-color:"+BM+";selection-color:"+B+";"
         "  line-height:1.9;}";

    // ═══ PushButton ═══
    q += "QPushButton{background:"+PW80+";color:"+IS+";border:1px solid "+PD40+";"
         "  border-radius:8px;padding:4px 14px;font-size:12px;}"
         "QPushButton:hover{background:"+PW+";color:"+I+";}"
         "QPushButton:pressed{background:"+PD+";}"
         "QPushButton:disabled{color:"+IG+";}"
         "QPushButton[flat=true]{background:transparent;border:none;padding:0;}"
         "QPushButton[flat=true]:hover{background:"+PW+"}";

    // ═══ CheckBox ═══
    q += "QCheckBox{color:"+IS+";font-size:12px;spacing:8px;}"
         "QCheckBox::indicator{width:12px;height:12px;border:2px solid "+IF+";"
         "  border-radius:4px;background:"+PW+";padding:2px;}"
         "QCheckBox::indicator:checked{background:"+B+";border-color:"+B+";}"
         "QCheckBox::indicator:hover{border-color:"+B+";}"
         "QCheckBox::indicator:checked:hover{background:"+BL+";border-color:"+BL+";}";

    // ═══ Slider ═══
    q += "QSlider::groove:horizontal{background:"+PD50+";height:5px;border-radius:3px;}"
         "QSlider::handle:horizontal{background:"+B+";width:16px;height:12px;"
         "  margin:-4px 0;border-radius:4px;}";

    // ═══ Splitter ═══
    q += "QSplitter::handle{background:"+PD30+";}";

    // ═══ Menu ═══
    q += "QMenu{background:"+CL95+";border:1px solid "+PD50+";border-radius:8px;padding:4px 0;}"
         "QMenu::item{padding:5px 16px;color:"+IS+";font-size:12px;}"
         "QMenu::item:selected{background:"+BM60+";color:"+B+";}"
         "QMenu::separator{height:1px;background:"+PD20+";margin:2px 8px;}"
         "QMenu QMenu::item{padding:5px 24px 5px 16px;}";

    // ═══ ComboBox ═══
    q += "QComboBox{background:"+PW80+";color:"+IS+";border:1px solid "+PD40+";"
         "  border-radius:6px;padding:3px 10px;font-size:12px;}"
         "QComboBox:hover{border-color:"+B+";}"
         "QComboBox::drop-down{border:none;width:20px;}";

    // ═══ ScrollArea ═══
    q += "QScrollArea{background:transparent;border:none;}"
         "QScrollArea>QWidget{background:transparent;}"
         "QScrollArea>QWidget>QWidget{background:transparent;}";

    // ═══ Frames / separators ═══
    q += "QFrame[frameShape=\"4\"],QFrame[frameShape=\"5\"]{color:"+PD30+";}";

    // ═══ Title bar (#titleBar) ═══
    // Tauri: h-11 (44px), bg-paper/60, border-b border-paper-deep/30
    q += "#titleBar{background:"+PP60+";border-bottom:1px solid "+PD30+";"
         "  min-height:44px;max-height:44px;}"
         // Default title bar buttons
         "#titleBar QPushButton{background:transparent;border:none;"
         "  font-size:13px;padding:0;color:"+IC+";border-radius:0;}"
         "#titleBar QPushButton:hover{background:"+PW+";color:"+ICH+";}"
         // Action buttons (📝, ⚙): w-10 h-11 (40x44)
         "#titleBar QPushButton#titleActionBtn{min-width:40px;min-height:44px;}"
         // Window control buttons (─, □, ✕): w-11 h-11 (44x44)
         "#titleBar QPushButton#winCtrlBtn,"
         "#titleBar QPushButton#closeBtn{min-width:44px;min-height:44px;}"
         // close button uses same hover as other window controls
         "#titleBar QLabel#appTitle{font-family:'Noto Serif SC','Source Han Serif SC',serif;"
         "  font-size:13px;font-weight:500;color:"+IS+";}"
         "#titleBar QLabel#subTitle{font-size:11px;color:"+IF+";}"
         "#titleBar QLabel#subSeparator{font-size:11px;color:"+IG+";}"
         "#titleBarDivider{background:"+PD30+";}";

    // ═══ Sidebar ═══
    // Bottom buttons: lighter solid colors, no gradient
    q += "#sidebarNoteCount{font-size:10px;color:"+IG+";"
         "  font-family:'JetBrains Mono','Fira Code',monospace;"
         "  letter-spacing:0.05em;text-transform:uppercase;padding:0 0 4px 0;"
         "  text-align:center;}"
         "#sidebarNewBtn{border:none;border-radius:8px;"
         "  font-size:13px;color:"+CL+";text-align:center;padding:10px 12px;"
         "  background:"+BL+";}"
         "#sidebarNewBtn:hover{background:"+NBH+";}"
         "#sidebarImportBtn{border:none;border-radius:8px;"
         "  font-size:13px;color:"+CL+";text-align:center;padding:10px 12px;"
         "  background:"+IF+";}"
         "#sidebarImportBtn:hover{background:"+IBH+";}";


    // ═══ Note items (#noteItem) ═══
    // Tauri: rounded-xl px-3 py-2.5 → enlarged: fixed height, no border, matching button width
    // Left indicator: thick straight bar via border-left with zero left radius
    q += "#noteItem{border:none;background:transparent;border-radius:12px;"
         "  text-align:left;padding:0;}"
         "#noteItem[noteInCategory=true]{border-radius:8px;"
         "  padding:0;margin:0 4px;}"
         "#noteItem QLabel#noteTitle{font-family:'Noto Serif SC','Source Han Serif SC',serif;"
         "  font-size:14px;font-weight:500;color:"+IS+";}"
         "#noteItem QLabel#notePreview{font-size:12px;color:"+IG+";}"
         "#noteItem QLabel#noteDate{font-size:11px;color:"+IG60+";"
         "  font-family:'JetBrains Mono','Fira Code',monospace;}"
         "#noteItem QLabel#noteTime{font-size:11px;color:"+IG60+";"
         "  font-family:'JetBrains Mono','Fira Code',monospace;}"
         "#noteItem QLabel#noteWordCount{font-size:11px;color:"+IG60+";"
         "  font-family:'JetBrains Mono','Fira Code',monospace;}"
         "#noteItem QLabel#noteDot{font-size:11px;color:"+IG40+";}"
         "#noteItem:hover{background:"+PW70+";}"
         // Selected state: indicator bar only, no border-left
         "#noteItem[noteSelected=true]{background:"+BM80+";"
         "  border-radius:12px;}"
         "#noteIndicator{background:"+B+";border-radius:2px;}"
         "#noteItem[noteSelected=true] #noteIndicator{background:"+B+";}"
         "#noteIndicator[indicatorOn=true]{background:"+BH60+";}";

    // ═══ Category header ═══
    // Tauri: bg-bamboo/8 border border-bamboo/15 rounded-lg (expanded: rounded-b-none)
    q += "#categoryHeader{background:"+BH8+";border:1px solid "+BH15+";"
         "  border-radius:8px;padding:4px 10px;}"
         // When expanded, remove bottom border radius
         "#categoryHeader[catExpanded=true]{border-radius:8px 8px 0 0;}"
         // When collapsed, full rounded corners
         "#categoryHeader[catExpanded=false]{border-radius:8px;}"
         "#categoryHeader QLabel#catName{font-size:11px;font-weight:500;color:"+BH70+";}"
         "#categoryHeader QLabel#catCount{font-size:9px;color:"+BH40+";"
         "  font-family:'JetBrains Mono','Fira Code',monospace;}"
         "#categoryHeader QLabel#catArrow{font-size:10px;color:"+BH50+";"
         "  background:transparent;}"
         "#categoryHeader QLabel#catIcon{background:transparent;}"
         // Tauri: bg-bamboo/[0.03] border border-t-0 border-bamboo/10 rounded-b-lg
         "#categoryBody{background:"+BH03+";border:1px solid "+BH10+";"
         "  border-top:0;border-radius:0 0 8px 8px;padding:2px 0;}";

    // ═══ Search box ═══
    q += "#searchBox{background:"+PW80+";border:1px solid "+PD40+";"
         "  border-radius:8px;padding:4px 10px 4px 4px;font-size:12px;color:"+I+";}"
         "#searchBox:focus{background:"+CL+";border-color:"+BH30+";}";

    // ═══ Format toolbar ═══
    q += "#formatToolbar QPushButton{background:transparent;border:none;"
         "  font-size:12px;padding:0;border-radius:5px;color:"+IG+";"
         "  min-width:28px;min-height:28px;max-width:28px;max-height:28px;}"
         "#formatToolbar QPushButton:hover{background:"+PW+";color:"+IF+";}";

    // ═══ Editor area ═══
    q += "#editorTitleInput{font-family:'Noto Serif SC','Source Han Serif SC',serif;"
         "  font-size:20px;font-weight:bold;color:"+I+";border:none;background:transparent;"
         "  padding:0;}"
         "#editorTitleInput:disabled{color:"+IG60+";}"
         "#metaInfoLabel{font-size:10px;color:"+IG+";"
         "  font-family:'JetBrains Mono','Fira Code',monospace;}"
         "#metaDot{font-size:10px;color:"+IG40+";}"
         "#saveStateLabel{font-size:10px;font-family:'JetBrains Mono','Fira Code',monospace;}";

    // ═══ Status bar ═══
    q += "#statusBar{background:"+PP30+";border-top:1px solid "+PD20+";"
         "  min-height:28px;max-height:28px;}"
         "#statusBar QLabel{font-size:10px;color:"+IG+";"
         "  font-family:'JetBrains Mono','Fira Code',monospace;}";

    // ═══ Settings panel ═══
    q += "#settingsPanel{background:"+CL92+";border-left:1px solid "+PD30+";}"
         "#settingsTitleBar{background:"+CL+";border-bottom:1px solid "+PD+";"
         "  min-height:44px;max-height:44px;}"
         "#settingsTitle{font-family:'Noto Serif SC','Source Han Serif SC',serif;"
         "  font-size:13px;font-weight:500;color:"+IS+";}"
         "#settingsSectionLabel{font-size:11px;color:"+IF+";margin-bottom:2px;}"
         "#settingsToggleRow{background:transparent;border:none;"
         "  padding:1px 4px;}"
         "#settingsToggleRow QLabel{font-size:12px;color:"+IS+";}"
         "#settingsDirInput{font-size:11px;color:"+IF+";background:"+PW70+";"
         "  border:1px solid "+PD40+";border-radius:8px;padding:4px 10px;"
         "  font-family:'JetBrains Mono','Fira Code',monospace;}";

    // ═══ Slider row ═══
    q += "#settingsSliderRow{background:"+PW45+";border:2px solid "+PD25+";"
         "  border-radius:8px;padding:8px 12px;}";

    // ═══ Notepad surface ═══
    // Tauri: noise-bg bg-cloud border border-paper-deep/40 rounded-xl shadow
    q += "#notepadCentral{background:"+(isDark()?P:QString("#f2f0e9"))+";}"
         "#notepadCard{background:"+CL+";border:none;border-radius:0;}"
         "#padMiniBar{background:transparent;}"
         // Separator — mx-4 mt-1 h-px bg-paper-deep/50
         "#padSeparator{background:"+PD50+";margin:4px 8px 0 8px;}"
         // Tab buttons (新建/打开)
         "#padTabBtn{font-size:13px;background:transparent;border:none;"
         "  padding:6px 14px;border-radius:6px;color:"+IG+";}"
         "#padTabBtn:hover{color:"+IF+";}"
         "#padTabBtn[tabActive=true]{color:"+B+";font-weight:500;}"
         // Indicator bar — 64×4 rounded rect under active tab
         "#padTabIndicator{background:"+B+";border-radius:1px;}"
         // Title input
         "#padTitleInput{font-family:'Noto Serif SC','Source Han Serif SC',serif;"
         "  font-size:14px;font-weight:500;color:"+I+";background:transparent;"
         "  border:none;padding:4px 8px;margin-bottom:8px;}"
         // Title-content separator — removed
         // Content textarea
         "#padContentArea{font-size:14px;font-family:'Noto Sans SC','Source Han Sans SC',sans-serif;"
         "  color:"+IS+";background:transparent;border:none;padding:0;line-height:1.7;}"
         // Bottom bar
         "#padBottomBar{background:transparent;}"
         "#padStatusLabel{font-size:11px;color:"+IG+";"
         "  font-family:'JetBrains Mono','Fira Code',monospace;}"
         // Clear button — Tauri: px-4 py-1.5 text-[12px] rounded-lg
         "#padClearBtn{background:transparent;border:none;font-size:12px;"
         "  color:"+IF+";padding:4px 16px;border-radius:8px;}"
         "#padClearBtn:hover{color:"+IS+";background:"+PW+";}"
         // Save button — Tauri: bg-bamboo text-cloud rounded-lg font-medium
         "#padSaveBtn{background:"+B+";color:"+CL+";font-size:12px;font-weight:500;"
         "  border:none;padding:4px 16px;border-radius:8px;}"
         "#padSaveBtn:hover{background:"+BL+";}"
         // Action buttons (pin, close)
         "#padActionBtn,#padCloseBtn{background:transparent;border:none;"
         "  color:"+IC+";font-size:13px;min-width:28px;min-height:28px;"
         "  max-width:28px;max-height:28px;border-radius:8px;}"
         "#padActionBtn:hover{background:"+PW+";color:"+ICH+";}"
         "#padCloseBtn:hover{color:#ef4444;background:"+DB+";}"
         // Note list items in notepad open mode now reuse #noteItem styles
         "#padEmptyLabel{font-size:12px;color:"+IG+";padding:32px 0;}"
         "#padOpenList{background:transparent;}"
         "#padCanvas{background:"+CL+";border:1px solid "+PD20+";border-radius:4px;margin-bottom:0;}";

    // ═══ Toolbar bar ═══
    q += "#toolbarBar{background:"+PP20+";border-bottom:1px solid "+PD20+";"
         "  min-height:40px;max-height:40px;}"
         // Toolbar icon buttons (sidebar toggle, pin, undo, trash)
         "#toolbarIconBtn{font-size:13px;color:"+IC+";background:transparent;"
         "  border:none;border-radius:8px;}"
         "#toolbarIconBtn:hover{background:"+PW+";}"
         "#toolbarIconBtn:hover{color:"+ICH+";}"
         "#toolbarIconBtn:disabled{color:"+IG40+";}"
         // Pin button gets bamboo hover
         "#pinTileBtn:hover{color:"+B+";background:"+BM50+";}"
         // Trash button gets danger hover
         "#trashBtn:hover{color:#ef4444;background:"+DB+";}"
         // Toolbar save icon button
         "#toolbarSaveBtn{font-size:13px;color:"+IC+";background:transparent;"
         "  border:none;border-radius:8px;}"
         "#toolbarSaveBtn:hover{background:"+PW+";}"
         "#toolbarSaveBtn:hover{color:"+ICH+";}"
         "#toolbarSaveBtn:disabled{color:"+IG40+";}"
         // Toolbar divider separator
         "#toolbarDivider{background:"+PD30+";margin:0 4px;}";
    q += "#editorMetaBar{border-bottom:1px solid "+PD20+";}";
    q += "#sidebarPanel{background:"+PP40+";border-right:1px solid "+PD30+";}";
    q += "#sidebarHandle{background:transparent;}"
         "#sidebarHandle:hover{background:"+BM60+";}";
    q += "#emptyStateLabel{font-size:13px;color:"+IG+";}";

    // ═══ Editor & Preview canvases ═══
    q += "#editorCanvas,#previewCanvas{background:transparent;border-radius:4px;margin:0 2px;border:1px solid "+PD20+";}"
         "#previewCanvas{padding:0;}"
         "#editorCanvas{padding:0;}"
         "#formatToolbar{margin-bottom:4px;}";

    // ═══ Settings panel border ═══
    q += "#settingsBorder{color:"+PD30+";}";

    qApp->setStyleSheet(q);

    // Set palette for proper SVG currentColor resolution
    QPalette pal = QApplication::palette();
    pal.setColor(QPalette::Window, p.paper);
    pal.setColor(QPalette::WindowText, p.ink);
    pal.setColor(QPalette::Base, p.paper);
    pal.setColor(QPalette::AlternateBase, p.paperWarm);
    pal.setColor(QPalette::Text, p.ink);
    pal.setColor(QPalette::Button, p.paperWarm);
    pal.setColor(QPalette::ButtonText, p.inkSoft);
    pal.setColor(QPalette::BrightText, p.ink);
    pal.setColor(QPalette::Highlight, p.bamboo);
    pal.setColor(QPalette::HighlightedText, p.cloud);
    pal.setColor(QPalette::ToolTipBase, p.paperWarm);
    pal.setColor(QPalette::ToolTipText, p.ink);
    QApplication::setPalette(pal);

    // Set default application font
    QFont appFont("Noto Sans SC");
    appFont.setStyleHint(QFont::SansSerif);
    QApplication::setFont(appFont);
}

QIcon FloralTheme::icon(const QString &svgPath) {
    auto &th = instance();
    QColor c = th.isDark() ? th.current().inkSoft : th.current().inkGhost;
    QFile f(svgPath);
    if (!f.open(QIODevice::ReadOnly)) return QIcon(svgPath);
    QByteArray data = f.readAll();
    f.close();
    QString s = QString::fromUtf8(data);
    // Replace currentColor with the resolved hex color
    s.replace("currentColor", c.name());
    QByteArray colored = s.toUtf8();

    QSvgRenderer renderer(colored);
    if (!renderer.isValid()) return QIcon(svgPath);

    // Render at 4x so HiDPI screens get crisp physical pixels
    // (12×12 logical → 48×48 physical @ DPR=4)
    QSize defaultSize = renderer.defaultSize();
    const int scale = 4;
    QPixmap pm(defaultSize * scale);
    pm.fill(Qt::transparent);
    QPainter painter(&pm);
    renderer.render(&painter);
    painter.end();
    pm.setDevicePixelRatio(scale);
    return QIcon(pm);
}
