#include "DaemonLauncher.h"

#include <QFileInfo>

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

bool DaemonLauncher::startDaemon()
{
    // Already running?
    if (isRunning())
        return true;

    // Validate path
    if (m_daemonPath.isEmpty()) {
        return false;
    }

    QFileInfo fi(m_daemonPath);
    if (!fi.exists() || !fi.isExecutable()) {
        return false;
    }

    m_process.setProgram(m_daemonPath);
    m_process.setArguments({});

    m_process.start();

    // Optional wait for start — keeps logic clean
    bool ok = m_process.waitForStarted(3000);
    return ok && isRunning();
}

bool DaemonLauncher::stopDaemon()
{
    if (!isRunning())
        return true; // already stopped

    m_process.terminate();

    if (!m_process.waitForFinished(3000)) {
        // Fallback: force kill
        m_process.kill();
        m_process.waitForFinished(2000);
    }

    return !isRunning();
}

bool DaemonLauncher::isRunning() const
{
    return m_process.state() == QProcess::Running;
}
