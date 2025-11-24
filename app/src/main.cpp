#include <QApplication>
#include <QFile>
#include <QDebug>

#include "MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // --- Optional: load QSS (later) ---
    // QFile styleFile(":/styles/main.qss");
    // if (styleFile.open(QFile::ReadOnly)) {
    //     QString style = QString::fromUtf8(styleFile.readAll());
    //     app.setStyleSheet(style);
    // }

    MainWindow w;
    w.show();          // normal show (alerts may minimize it)

    return app.exec();
}
