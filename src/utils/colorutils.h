// ColorUtils — 替代 chroma-js 的颜色混合工具
#ifndef COLORUTILS_H
#define COLORUTILS_H

#include <QColor>
#include <QString>

namespace ColorUtils {

// 解析颜色字符串，支持 #RGB #RRGGBB #RRGGBBAA
QColor parseColor(const QString &hex);

// 混合两个颜色，ratio 0.0 → c1, 1.0 → c2
QColor mix(const QColor &c1, const QColor &c2, double ratio);

// 混合颜色与目标色，返回带 alpha 的结果
QColor mixAlpha(const QColor &base, const QColor &target, double ratio, double alpha);

// 计算相对亮度 (0-1)
double luminance(const QColor &color);

// 判断是否为浅色背景
bool isLight(const QColor &color, double threshold = 0.18);

} // namespace ColorUtils

#endif // COLORUTILS_H
