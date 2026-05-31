#include "settingspanel.h"
#include "widgets/slidingbuttongroup.h"
#include "app/theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QSlider>
#include <QGroupBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QComboBox>
#include <QFileDialog>
#include <QColorDialog>
#include <QFocusEvent>
#include "widgets/styledcheckbox.h"

SettingsPanel::SettingsPanel(QWidget *parent) : QWidget(parent) {
    setMinimumWidth(0);
    setFixedWidth(360);
    buildUI();
}

void SettingsPanel::buildUI() {
    setObjectName("settingsPanel");
    auto *ml = new QVBoxLayout(this);
    ml->setContentsMargins(0,0,0,0); ml->setSpacing(0);

    auto *sc = new QScrollArea; sc->setWidgetResizable(true); sc->setFrameShape(QFrame::NoFrame);
    auto *sw = new QWidget;
    auto *l = new QVBoxLayout(sw); l->setSpacing(12); l->setContentsMargins(16,12,16,20);

    auto addLabel = [&](const QString &t) {
        auto *lb = new QLabel(t); lb->setObjectName("settingsSectionLabel");
        l->addWidget(lb);
    };

    // 夜间模式
    addLabel("夜间模式");
    m_nightModeGroup = new SlidingButtonGroup;
    m_nightModeGroup->setOptions({{"system","跟随系统"},{"light","浅色"},{"dark","深色"}});
    connect(m_nightModeGroup, &SlidingButtonGroup::valueChanged, this, [this](const QString &v){ m_config.nightMode=v; emit configChanged(m_config); });
    l->addWidget(m_nightModeGroup);

    // Markdown 主题
    addLabel("Markdown 主题");
    m_markdownThemeGroup = new SlidingButtonGroup;
    m_markdownThemeGroup->setOptions({{"github","GitHub"},{"claude","Claude"}});
    connect(m_markdownThemeGroup, &SlidingButtonGroup::valueChanged, this, [this](const QString &v){ m_config.markdownTheme=v; emit configChanged(m_config); });
    l->addWidget(m_markdownThemeGroup);

    // 笔记目录
    addLabel("笔记目录");
    auto *dirRow = new QHBoxLayout;
    m_notesDirEdit = new QLineEdit;
    m_notesDirEdit->setStyleSheet(
        "font-size:11px;background:transparent;border:none;outline:none;"
        "font-family:'JetBrains Mono','Fira Code',monospace;");
    dirRow->addWidget(m_notesDirEdit);
    m_dirBtn = new QPushButton("浏览"); m_dirBtn->setFixedHeight(28);
    connect(m_dirBtn, &QPushButton::clicked, this, [this]() {
        if (m_notesDirEdit->hasFocus()) {
            m_notesDirEdit->clearFocus();
            m_dirBtn->setText("浏览");
        } else {
            QString d = QFileDialog::getExistingDirectory(this,"选择笔记目录",m_notesDirEdit->text());
            if(!d.isEmpty()){ m_notesDirEdit->setText(d); m_config.notesDir=d; emit configChanged(m_config); }
        }
    });
    connect(m_notesDirEdit, &QLineEdit::textChanged, this, [this](const QString &t){
        m_config.notesDir=t; emit configChanged(m_config);
    });
    // Switch button text on focus
    connect(m_notesDirEdit, &QLineEdit::editingFinished, this, [this](){ m_dirBtn->setText("浏览"); });
    // Focus-in: switch to 确定 mode
    m_notesDirEdit->installEventFilter(this);
    dirRow->addWidget(m_dirBtn);
    l->addLayout(dirRow);

    // 快捷键
    addLabel("快捷键");
    m_shortcutCombo = new QComboBox;
    m_shortcutCombo->addItems({"Ctrl+Space","Alt+Space","Ctrl+Shift+N","Ctrl+Alt+N"});
    connect(m_shortcutCombo, &QComboBox::currentTextChanged, this, [this](const QString &v){ m_config.globalShortcut=v; emit configChanged(m_config); });
    l->addWidget(m_shortcutCombo);

    // 开关 — Tauri ToggleRow: bg-paper-warm/45 border-paper-deep/25 rounded-lg
    auto addToggle = [&](const QString &t, bool *f){
        auto *row = new QWidget;
        row->setObjectName("settingsToggleRow");
        auto *rl = new QHBoxLayout(row);
        rl->setContentsMargins(4,0,4,0);
        auto *cb = new StyledCheckBox(t); cb->setChecked(*f);
        connect(cb, &StyledCheckBox::toggled, this, [this, f](bool v){ *f=v; emit configChanged(m_config); });
        rl->addWidget(cb);
        l->addWidget(row);
    };
    addToggle("关闭到托盘",&m_config.closeToTray);
    addToggle("开机自启",&m_config.autostart);
    addToggle("自动保存笔记",&m_config.noteAutoSave);
    addToggle("小窗笔记自动保存",&m_config.noteSurfaceAutoSave);

    // 字号 — Tauri: bg-paper-warm/45 border-paper-deep/25 rounded-lg
    auto addSlider = [&](const QString &t, int *field){
        addLabel(t);
        auto *row = new QWidget;
        row->setObjectName("settingsSliderRow");
        auto *rl = new QHBoxLayout(row);
        rl->setContentsMargins(4,0,4,0);
        auto *sl = new QSlider(Qt::Horizontal); sl->setRange(8,30); sl->setValue(*field);
        connect(sl, &QSlider::valueChanged, this, [this, field](int v){ *field=v; emit configChanged(m_config); });
        rl->addWidget(sl, 1);
        rl->addSpacing(8);
        auto *val = new QLabel(QString::number(*field)+"px");
        val->setStyleSheet(QString("font-size:12px;font-family:'JetBrains Mono','Fira Code',monospace;color:%1;").arg(
            FloralTheme::instance().current().inkSoft.name()));
        val->setFixedWidth(35);
        connect(sl, &QSlider::valueChanged, val, [val](int v){ val->setText(QString::number(v)+"px"); });
        rl->addWidget(val);
        l->addWidget(row);
    };
    addSlider("编辑器字号",&m_config.fontSize);
    addSlider("小窗/磁贴字号",&m_config.surfaceFontSize);

    l->addStretch();
    sc->setWidget(sw);
    ml->addWidget(sc);
}

void SettingsPanel::loadConfig(const AppConfig &config) {
    m_config = config;
    m_nightModeGroup->setCurrentValue(config.nightMode);
    m_markdownThemeGroup->setCurrentValue(config.markdownTheme);
    m_notesDirEdit->setText(config.notesDir);
    m_notesDirEdit->setCursorPosition(0);
    m_shortcutCombo->setCurrentText(config.globalShortcut);
    if (auto *sc = findChild<QScrollArea*>())
        QTimer::singleShot(0, sc, [sc](){ sc->verticalScrollBar()->setValue(0); });
}

bool SettingsPanel::eventFilter(QObject *obj, QEvent *event) {
    if (obj == m_notesDirEdit && event->type() == QEvent::FocusIn) {
        m_dirBtn->setText("确定");
        return false;
    }
    return QWidget::eventFilter(obj, event);
}
