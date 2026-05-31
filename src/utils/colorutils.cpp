#include "colorutils.h"
#include <cmath>

namespace ColorUtils {

QColor parseColor(const QString &hex) {
    return QColor(hex);
}

QColor mix(const QColor &c1, const QColor &c2, double ratio) {
    ratio = std::clamp(ratio, 0.0, 1.0);
    double r = c1.redF()   * (1.0 - ratio) + c2.redF()   * ratio;
    double g = c1.greenF() * (1.0 - ratio) + c2.greenF() * ratio;
    double b = c1.blueF()  * (1.0 - ratio) + c2.blueF()  * ratio;
    return QColor::fromRgbF(r, g, b);
}

QColor mixAlpha(const QColor &base, const QColor &target, double ratio, double alpha) {
    QColor mixed = mix(base, target, ratio);
    mixed.setAlphaF(std::clamp(alpha, 0.0, 1.0));
    return mixed;
}

double luminance(const QColor &color) {
    // Relative luminance per WCAG 2.0
    auto linearize = [](double c) -> double {
        c /= 255.0;
        return c <= 0.03928 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * linearize(color.red()) +
           0.7152 * linearize(color.green()) +
           0.0722 * linearize(color.blue());
}

bool isLight(const QColor &color, double threshold) {
    return luminance(color) > threshold;
}

} // namespace ColorUtils
