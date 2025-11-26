#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

#include "UIDebugLog.h"

#include "MainWindow.h"

int main(int argc, char* argv[])
{
    ui_debug_log("=== UI Application START ===");

    QApplication app(argc, argv);

    // ----------------------------------------------------
    // 1. Determine UI PID file location
    // ----------------------------------------------------
    QString exePath = QCoreApplication::applicationFilePath();
    QString exeDir  = QFileInfo(exePath).absolutePath();
    QString uiPidFile = exeDir + "/ui.pid";

    // ----------------------------------------------------
    // 2. Write current PID to ui.pid
    // ----------------------------------------------------
    {
        QFile f(uiPidFile);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qWarning() << "Failed to write ui.pid";
        } else {
            f.write(QString::number(QCoreApplication::applicationPid()).toUtf8());
            f.close();
        }
    }

    // ----------------------------------------------------
    // 3. Ensure ui.pid is cleared on app exit
    // ----------------------------------------------------
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&]() {
        QFile f(uiPidFile);
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            f.write("");   // erase pid
            f.close();
        }
    });

    // ----------------------------------------------------
    // Launch main window
    // ----------------------------------------------------
    MainWindow w;
    w.show();

    int view = app.exec();

    ui_debug_log("=== UI Application EXIT ===");

    return view;
}
