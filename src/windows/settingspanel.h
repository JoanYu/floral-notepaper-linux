// SettingsPanel — 设置面板 (对应原 SettingsPanel.tsx)
#ifndef SETTINGSPANEL_H
#define SETTINGSPANEL_H

#include <QWidget>
#include "services/configmanager.h"

class QLineEdit;
class QPushButton;
class QComboBox;
class SlidingButtonGroup;

class SettingsPanel : public QWidget {
    Q_OBJECT
public:
    explicit SettingsPanel(QWidget *parent = nullptr);

    void loadConfig(const AppConfig &config);
    AppConfig currentConfig() const { return m_config; }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

signals:
    void configChanged(const AppConfig &config);
    void chooseNotesDirRequested();
    void closeRequested();

private:
    void buildUI();
    void applyConfig();

    AppConfig m_config;
    SlidingButtonGroup *m_nightModeGroup = nullptr;
    SlidingButtonGroup *m_markdownThemeGroup = nullptr;
    SlidingButtonGroup *m_viewModeGroup = nullptr;
    QLineEdit *m_notesDirEdit = nullptr;
    QPushButton *m_dirBtn = nullptr;
    QComboBox *m_shortcutCombo = nullptr;
};

#endif // SETTINGSPANEL_H
