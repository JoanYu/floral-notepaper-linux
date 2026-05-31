// ConfigManager — 配置读写 (对应原 config.json)
#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QString>
#include <QJsonObject>

struct AppConfig {
    QString notesDir;
    QString globalShortcut = "Ctrl+Space";
    bool closeToTray = true;
    bool autostart = false;
    QString defaultViewMode = "edit";   // edit / split / preview
    bool noteAutoSave = true;
    bool noteSurfaceAutoSave = true;
    QString tileColor = "#f6f3ec";
    QString tileColorMode = "system";   // system / custom
    QString nightMode = "system";       // light / dark / system
    QString markdownTheme = "github";    // github / claude
    int fontSize = 14;
    int surfaceFontSize = 14;

    QJsonObject toJson() const;
    static AppConfig fromJson(const QJsonObject &obj);
};

class ConfigManager {
public:
    explicit ConfigManager(const QString &baseDir);

    AppConfig load() const;
    void save(const AppConfig &config);

    QString baseDir() const { return m_baseDir; }

private:
    QString m_baseDir;
    QString configPath() const;
    AppConfig defaultConfig() const;
};

#endif // CONFIGMANAGER_H
