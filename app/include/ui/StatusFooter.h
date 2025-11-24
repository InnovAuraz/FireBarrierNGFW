#pragma once

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>

class StatusFooter : public QWidget
{
    Q_OBJECT

public:
    explicit StatusFooter(QWidget* parent = nullptr);

public slots:
    void updateDaemonAlive(bool alive);
    void updatePythonOnline(bool online);
    void updateCaptureRunning(bool running);
    void updateModelVersion(const QString& version);
    void updateLastError(const QString& error);

private:
    QLabel* daemonLabel = nullptr;
    QLabel* pythonLabel = nullptr;
    QLabel* captureLabel = nullptr;
    QLabel* modelLabel = nullptr;
    QLabel* errorLabel = nullptr;

    void updateLabel(QLabel* lbl, bool ok, const QString& name);
};
