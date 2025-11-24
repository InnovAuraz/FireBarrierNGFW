#pragma once

#include <QObject>
#include <QString>
#include <QVector>

#include "UiConfigLoader.h"
#include "DaemonLauncher.h"
#include "DaemonClient.h"
#include "EventSubscriber.h"
#include "JsonProtocol.h"

// UiController is the MAIN orchestrator for FireBarrier UI.
// It:
//   - loads ui_config.json
//   - manages daemon start/stop
//   - sends REQ actions (status/stats/flows)
//   - listens to daemon PUB events
//   - parses JSON responses
//   - emits high-level signals for MainWindow

class UiController : public QObject
{
    Q_OBJECT

public:
    explicit UiController(QObject *parent = nullptr);
    ~UiController();

    // ---- Initialization ----
    bool initialize(); // loads config, sets endpoints, starts subscriber

    // ---- Manual daemon control ----
    bool startDaemon();
    bool stopDaemon();

    // ---- REQ actions (async) ----
    void refreshStatus();
    void refreshStats();
    void refreshFlows();

    // ---- Accessors ----
    QString daemonPath() const { return m_daemonPath; }

signals:
    // UI should update status widgets
    void statusUpdated(const UIStatusPayload &status);

    // UI should update stats widgets
    void statsUpdated(const UIStatsPayload &stats);

    // UI should update flows list/table
    void flowsUpdated(const QVector<FlowSnapshot> &flows);

    // Daemon events from PUB/SUB
    void daemonAlertReceived(const DaemonEvent &evt);
    void daemonErrorReceived(const DaemonEvent &evt);
    void daemonModelUpdateReceived(const DaemonEvent &evt);
    void daemonStatusChangeReceived(const DaemonEvent &evt);

    // Lifecycle / error notifications
    void daemonStarted();
    void daemonStopped();
    void controllerError(const QString &message);

private slots:
    // Internal handlers
    void onRequestCompleted(const QString &action, const QByteArray &json);
    void onRequestFailed(const QString &action, const QString &error);

    void onEventReceived(const QByteArray &json);
    void onSubscriberError(const QString &error);

private:
    // internal helpers
    void processUiResponse(const UiResponse &resp);
    void processDaemonEvent(const DaemonEvent &evt);

private:
    // Config
    UiConfigLoader m_config;
    QString m_daemonPath;

    // Subsystems
    DaemonLauncher m_launcher;
    DaemonClient   m_client;
    EventSubscriber m_subscriber;

    // Endpoints from config
    QString m_reqEndpoint = "tcp://127.0.0.1:6001";
    QString m_subEndpoint = "tcp://127.0.0.1:6002";
};
