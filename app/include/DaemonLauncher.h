#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

// Simple launcher for the daemon process.
// UIController will own this and call start/stop manually.
// No signals, no auto-restart, no async behavior required.
class DaemonLauncher : public QObject
{
    Q_OBJECT
public:
    explicit DaemonLauncher(QObject *parent = nullptr);

    // Set the path to the daemon executable
    void setDaemonPath(const QString &path);
    QString daemonPath() const;

    // Starts the daemon process
    // Returns true if started successfully
    bool startDaemon();

    // Attempts to gracefully stop daemon
    // Returns true if terminated or not running
    bool stopDaemon();

    // True if QProcess exists and is running
    bool isRunning() const;

private:
    QString m_daemonPath;
    QProcess m_process;
};
