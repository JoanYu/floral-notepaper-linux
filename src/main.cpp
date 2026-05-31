#include "app/application.h"
#include "windows/mainwindow.h"
#include "services/notestore.h"
#include "services/configmanager.h"
#include <QScreen>
#include <QGuiApplication>
#include <QDebug>
#include <cstdlib>

int main(int argc, char *argv[])
{
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) qputenv("QT_QPA_PLATFORM", "xcb");
    qputenv("DDE_DXCB_DISABLE", "1");
    qputenv("QT_QPA_PLATFORMTHEME", "none");
    qputenv("QT_STYLE_OVERRIDE", "Fusion");
    qputenv("DDE_DISABLE_QT6_INTEGRATION", "1");
    qputenv("DDE_DISABLE_QT5_INTEGRATION", "1");

    AppContext &ctx = AppContext::instance();
    ctx.initialize(argc, argv);

    QString dataDir = ctx.dataDir();
    NoteStore store(dataDir);
    ConfigManager configMgr(dataDir);

    MainWindow window;
    window.initialize(&store, &configMgr);

    if (auto *screen = QGuiApplication::primaryScreen()) {
        QRect geo = screen->availableGeometry();
        window.move((geo.width()-window.width())/2, (geo.height()-window.height())/2);
    }
    window.show();
    qDebug() << "花笺启动完成, 数据目录:" << dataDir;
    return ctx.run();
}
