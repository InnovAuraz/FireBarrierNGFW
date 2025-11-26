#pragma once

#include <QObject>
#include <QString>
#include <QVector>

#include "UiConfigLoader.h"
#include "DaemonLauncher.h"
#include "DaemonClient.h"
#include "JsonProtocol.h"

// UiController orchestrates everything the UI does.
// Responsibilities:
//   - Load ui_config.json
//   - Start/stop daemon via DaemonLauncher
//   - Communicate with daemon via ONE PAIR0 socket (DaemonClient)
//   - Parse UiResponse and DaemonEvent
//   - Emit high-level signals for the UI
class UiController : public QObject
{
    Q_OBJECT

public:
    explicit UiController(QObject *parent = nullptr);
    ~UiController();

    // Initialization (load config, set daemon path, set PAIR0 endpoint)
    bool initialize();

    // Manual daemon control
    bool startDaemon();
    bool stopDaemon();

    // UI → Daemon requests
    void refreshStatus();
    void refreshStats();
    void refreshFlows();

    QString daemonPath() const { return m_daemonPath; }

signals:
    // Parsed responses
    void statusUpdated(const UIStatusPayload &status);
    void statsUpdated(const UIStatsPayload &stats);
    void flowsUpdated(const QVector<FlowSnapshot> &flows);

    // Daemon events
    void daemonAlertReceived(const DaemonEvent &evt);
    void daemonErrorReceived(const DaemonEvent &evt);
    void daemonModelUpdateReceived(const DaemonEvent &evt);
    void daemonStatusChangeReceived(const DaemonEvent &evt);

    // Lifecycle + error
    void daemonStarted();
    void daemonStopped();
    void controllerError(const QString &message);

private slots:
    // Signals from DaemonClient
    void onUiResponseReceived(const UiResponse &resp);
    void onDaemonEventReceived(const DaemonEvent &evt);
    void onConnectionError(const QString &error);

private:
    void processUiResponse(const UiResponse &resp);
    void processDaemonEvent(const DaemonEvent &evt);

private:
    // Config
    UiConfigLoader m_config;
    QString m_daemonPath;

    // Subsystems
    DaemonLauncher m_launcher;
    DaemonClient   m_client;

    // Single PAIR0 endpoint (daemon UI channel)
    QString m_reqEndpoint = "tcp://127.0.0.1:6001";
};
