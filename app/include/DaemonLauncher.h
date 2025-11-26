#pragma once

#include <QObject>
#include <QString>

class DaemonLauncher : public QObject
{
    Q_OBJECT
public:
    explicit DaemonLauncher(QObject *parent = nullptr);

    void setDaemonPath(const QString &path);
    QString daemonPath() const;

    bool startDaemon();
    bool stopDaemon();
    bool isRunning() const;

private:
    bool writePidFile(qint64 pid);
    bool readPidFile(qint64 &pidOut) const;
    bool clearPidFile();
    QString pidFilePath() const;

private:
    QString m_daemonPath;
};
