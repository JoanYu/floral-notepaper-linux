// FloralTheme — 原项目 CSS Token → QSS 全局主题
#ifndef FLORAL_THEME_H
#define FLORAL_THEME_H

#include <QColor>
#include <QString>
#include <QApplication>
#include <QObject>

struct FloralPalette {
    QColor paper;
    QColor paperWarm;
    QColor paperDeep;
    QColor ink;
    QColor inkSoft;
    QColor inkFaint;
    QColor inkGhost;
    QColor bamboo;
    QColor bambooLight;
    QColor bambooMist;
    QColor bambooGlow;
    QColor stone;
    QColor cloud;
    QColor shadow;
    QColor shadowDeep;
    QColor dangerBg;

    static FloralPalette light();
    static FloralPalette dark();
};

enum class FloralThemeOption { Light, Dark, System };

class FloralTheme : public QObject {
	Q_OBJECT
public:
    static FloralTheme &instance();

    void apply(FloralThemeOption option);
    FloralThemeOption option() const { return m_option; }
    const FloralPalette &current() const { return m_palette; }
    bool isDark() const;

    static QIcon icon(const QString &svgPath);

signals:
    void themeChanged();

private:
    FloralTheme();
    void applyPalette();

    FloralThemeOption m_option = FloralThemeOption::System;
    FloralPalette m_palette;
};

#endif
