#include "DaemonLauncher.h"
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QDir>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

DaemonLauncher::DaemonLauncher(QObject *parent)
    : QObject(parent)
{
}

void DaemonLauncher::setDaemonPath(const QString &path)
{
    m_daemonPath = path;
}

QString DaemonLauncher::daemonPath() const
{
    return m_daemonPath;
}

QString DaemonLauncher::pidFilePath() const
{
    // Store pid next to the exe, named "daemon.pid"
    QString baseDir = QFileInfo(m_daemonPath).absolutePath();
    return baseDir + "/daemon.pid";
}

bool DaemonLauncher::writePidFile(qint64 pid)
{
    QFile f(pidFilePath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    f.write(QString::number(pid).toUtf8());
    return true;
}

bool DaemonLauncher::clearPidFile()
{
    QFile f(pidFilePath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    // Write empty content
    f.write("");
    return true;
}

bool DaemonLauncher::readPidFile(qint64 &pidOut) const
{
    QFile f(pidFilePath());
    if (!f.exists())
        return false;

    if (!f.open(QIODevice::ReadOnly))
        return false;

    QByteArray data = f.readAll().trimmed();
    if (data.isEmpty())
        return false;

    bool ok = false;
    qint64 pid = data.toLongLong(&ok);
    if (!ok || pid <= 0)
        return false;

    pidOut = pid;
    return true;
}

bool DaemonLauncher::isRunning() const
{
    qint64 pid = 0;
    if (!readPidFile(pid))
        return false;

    #ifdef Q_OS_WIN
        HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, (DWORD)pid);
        if (!h)
            return false;

        DWORD exitCode = 0;
        BOOL ok = GetExitCodeProcess(h, &exitCode);
        CloseHandle(h);

        return ok && exitCode == STILL_ACTIVE;
    #else
        // POSIX: if kill(pid, 0) == 0 -> process exists
        return (::kill(pid, 0) == 0);
    #endif
}

bool DaemonLauncher::startDaemon()
{
    // If already running -> do not start a new one
    if (isRunning())
        return true;

    if (m_daemonPath.isEmpty())
        return false;

    QFileInfo fi(m_daemonPath);
    if (!fi.exists() || !fi.isExecutable())
        return false;

#ifdef Q_OS_WIN
    QString exePath = m_daemonPath;
    if (!exePath.endsWith(".exe"))
        exePath += ".exe";
#else
    QString exePath = m_daemonPath;
#endif

    qint64 pid = 0;
    bool ok = QProcess::startDetached(exePath, {}, {}, &pid);

    if (!ok || pid <= 0)
        return false;

    // Persist PID
    writePidFile(pid);

    return true;
}

bool DaemonLauncher::stopDaemon()
{
    qint64 pid = 0;
    if (!readPidFile(pid))
        return true; // nothing to stop

    #ifdef Q_OS_WIN
        // Kill via Windows command
        QString program = "taskkill";
        QStringList args = { "/PID", QString::number(pid), "/T", "/F" };

        QProcess::execute(program, args);
    #else
        ::kill(pid, SIGTERM);
    #endif

    clearPidFile();
    return true;
}
