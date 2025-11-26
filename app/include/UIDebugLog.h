#pragma once
#include <QFile>
#include <QDateTime>
#include <QTextStream>
#include <QMutex>

inline void ui_debug_log(const QString& msg)
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);

    QFile f("ui.log");
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream ts(&f);
        ts << "[" << QDateTime::currentDateTime().toString("hh:mm:ss.zzz")
           << "] " << msg << "\n";
    }
}
