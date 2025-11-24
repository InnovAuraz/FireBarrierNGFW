#pragma once

#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>

class SystemStatusPage : public QWidget
{
    Q_OBJECT

public:
    explicit SystemStatusPage(QWidget* parent = nullptr);

public slots:
    void updateStatusPayload(bool daemonAlive,
                             bool pythonOnline,
                             bool captureRunning,
                             quint64 uptimeSec,
                             quint64 flowsProcessed,
                             const QString& modelVersion,
                             const QString& lastError);

private:
    QLabel* daemonLabel = nullptr;
    QLabel* pythonLabel = nullptr;
    QLabel* captureLabel = nullptr;
    QLabel* uptimeLabel = nullptr;
    QLabel* flowsLabel = nullptr;
    QLabel* modelLabel = nullptr;
    QLabel* errorLabel = nullptr;

    QLabel* makeValueLabel(const QString& text);
};
