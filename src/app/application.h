#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>
#include <QString>

class AppContext {
public:
    static AppContext &instance();
    void initialize(int &argc, char **argv);
    int run();
    QApplication *app() const { return m_app; }
    QString dataDir() const { return m_dataDir; }
private:
    AppContext() = default;
    QApplication *m_app = nullptr;
    QString m_dataDir;
};

#endif
