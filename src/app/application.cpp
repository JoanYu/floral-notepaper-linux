#include "application.h"
#include "theme.h"
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>

static void msgHandler(QtMsgType type, const QMessageLogContext &, const QString &msg) {
    const char *lvl = "INFO";
    if (type==QtDebugMsg) lvl="DEBUG";
    else if (type==QtWarningMsg) lvl="WARN";
    else if (type==QtCriticalMsg) lvl="ERROR";
    else if (type==QtFatalMsg) lvl="FATAL";
    fprintf(stderr, "%s [%s] %s\n",
            QDateTime::currentDateTime().toString("hh:mm:ss.zzz").toUtf8().constData(),
            lvl, msg.toUtf8().constData());
    if (type==QtFatalMsg) abort();
}

AppContext &AppContext::instance() {
    static AppContext ctx;
    return ctx;
}

void AppContext::initialize(int &argc, char **argv) {
    m_app = new QApplication(argc, argv);
    m_app->setApplicationName("花笺");
    m_app->setOrganizationName("floral-note");
    m_app->setApplicationVersion("1.0.0");

    qInstallMessageHandler(msgHandler);

    m_dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (m_dataDir.isEmpty())
        m_dataDir = QDir::homePath() + "/.local/share/floral-note";
    QDir().mkpath(m_dataDir);

    FloralTheme::instance().apply(FloralThemeOption::System);
}

int AppContext::run() { return m_app->exec(); }
