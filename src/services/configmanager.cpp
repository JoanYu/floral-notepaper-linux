#include "configmanager.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QDebug>

// --- AppConfig ---

QJsonObject AppConfig::toJson() const {
    return {
        {"notesDir", notesDir},
        {"globalShortcut", globalShortcut},
        {"closeToTray", closeToTray},
        {"autostart", autostart},
        {"defaultViewMode", defaultViewMode},
        {"noteAutoSave", noteAutoSave},
        {"noteSurfaceAutoSave", noteSurfaceAutoSave},
        {"tileColor", tileColor},
        {"tileColorMode", tileColorMode},
        {"nightMode", nightMode},
        {"markdownTheme", markdownTheme},
        {"fontSize", fontSize},
        {"surfaceFontSize", surfaceFontSize},
    };
}

AppConfig AppConfig::fromJson(const QJsonObject &obj) {
    AppConfig c;
    c.notesDir          = obj.value("notesDir").toString(c.notesDir);
    c.globalShortcut    = obj.value("globalShortcut").toString(c.globalShortcut);
    c.closeToTray       = obj.value("closeToTray").toBool(c.closeToTray);
    c.autostart         = obj.value("autostart").toBool(c.autostart);
    c.defaultViewMode   = obj.value("defaultViewMode").toString(c.defaultViewMode);
    c.noteAutoSave      = obj.value("noteAutoSave").toBool(c.noteAutoSave);
    c.noteSurfaceAutoSave = obj.value("noteSurfaceAutoSave").toBool(c.noteSurfaceAutoSave);
    c.tileColor         = obj.value("tileColor").toString(c.tileColor);
    c.tileColorMode     = obj.value("tileColorMode").toString(c.tileColorMode);
    c.nightMode         = obj.value("nightMode").toString(c.nightMode);
    c.markdownTheme     = obj.value("markdownTheme").toString(c.markdownTheme);
    c.fontSize          = obj.value("fontSize").toInt(c.fontSize);
    c.surfaceFontSize   = obj.value("surfaceFontSize").toInt(c.surfaceFontSize);
    return c;
}

// --- ConfigManager ---

ConfigManager::ConfigManager(const QString &baseDir)
    : m_baseDir(baseDir)
{
    QDir().mkpath(m_baseDir);
}

QString ConfigManager::configPath() const {
    return m_baseDir + "/config.json";
}

AppConfig ConfigManager::defaultConfig() const {
    AppConfig c;
    c.notesDir = m_baseDir + "/notes";
    return c;
}

AppConfig ConfigManager::load() const {
    QFile f(configPath());
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) {
        auto def = defaultConfig();
        // Save default so it exists next time
        const_cast<ConfigManager*>(this)->save(def);
        return def;
    }
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (doc.isNull()) return defaultConfig();
    return AppConfig::fromJson(doc.object());
}

void ConfigManager::save(const AppConfig &config) {
    QDir().mkpath(m_baseDir);
    QDir().mkpath(config.notesDir);

    QFile f(configPath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QJsonDocument doc(config.toJson());
        f.write(doc.toJson(QJsonDocument::Indented));
        f.close();
    }
}
