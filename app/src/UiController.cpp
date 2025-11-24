#include "UiController.h"

#include <QDebug>

// ----------------------------------------
// Constructor / Destructor
// ----------------------------------------

UiController::UiController(QObject *parent)
    : QObject(parent),
      m_launcher(this),
      m_client(this),
      m_subscriber(this)
{
    // Connect DaemonClient signals
    connect(&m_client, &DaemonClient::requestCompleted,
            this, &UiController::onRequestCompleted);

    connect(&m_client, &DaemonClient::requestFailed,
            this, &UiController::onRequestFailed);

    // Connect EventSubscriber signals
    connect(&m_subscriber, &EventSubscriber::eventReceived,
            this, &UiController::onEventReceived);

    connect(&m_subscriber, &EventSubscriber::subscriberError,
            this, &UiController::onSubscriberError);
}

UiController::~UiController()
{
    // Ensure subscriber is stopped before destruction
    m_subscriber.stop();
    // Daemon may be left running intentionally; we do not force kill here
}

// ----------------------------------------
// Initialization
// ----------------------------------------

bool UiController::initialize()
{
    UiConfig config;
    QString errorMsg;

    if (!UiConfigLoader::load(config, &errorMsg)) {
        emit controllerError(QStringLiteral("Failed to load ui_config.json: %1").arg(errorMsg));
        return false;
    }

    m_daemonPath = config.daemonPath;
    m_launcher.setDaemonPath(m_daemonPath);

    // For now endpoints are fixed; in future they could be in config as well
    m_client.setEndpoint(m_reqEndpoint);
    m_subscriber.setEndpoint(m_subEndpoint);

    // Start subscriber (PUB/SUB listener)
    if (!m_subscriber.start()) {
        emit controllerError(QStringLiteral("Failed to start event subscriber"));
        return false;
    }

    return true;
}

// ----------------------------------------
// Daemon control
// ----------------------------------------

bool UiController::startDaemon()
{
    if (m_daemonPath.isEmpty()) {
        emit controllerError(QStringLiteral("Daemon path not configured"));
        return false;
    }

    if (m_launcher.isRunning()) {
        // Already running: still emit signal so UI stays in sync
        emit daemonStarted();
        return true;
    }

    if (!m_launcher.startDaemon()) {
        emit controllerError(QStringLiteral("Failed to start daemon process at: %1")
                             .arg(m_daemonPath));
        return false;
    }

    emit daemonStarted();
    return true;
}

bool UiController::stopDaemon()
{
    if (!m_launcher.isRunning()) {
        // Already stopped
        emit daemonStopped();
        return true;
    }

    if (!m_launcher.stopDaemon()) {
        emit controllerError(QStringLiteral("Failed to stop daemon process"));
        return false;
    }

    emit daemonStopped();
    return true;
}

// ----------------------------------------
// REQ actions (async, via DaemonClient)
// ----------------------------------------

void UiController::refreshStatus()
{
    m_client.requestStatus();
}

void UiController::refreshStats()
{
    m_client.requestStats();
}

void UiController::refreshFlows()
{
    m_client.requestFlows();
}

// ----------------------------------------
// Internal slots: REQ/REP handling
// ----------------------------------------

void UiController::onRequestCompleted(const QString &action, const QByteArray &json)
{
    UiResponse resp;
    QString error;

    if (!JsonProtocol::parseUiResponse(json, resp, &error)) {
        emit controllerError(QStringLiteral("Failed to parse ui_response for action '%1': %2")
                             .arg(action, error));
        return;
    }

    // Optional sanity: ensure action matches
    if (!resp.action.isEmpty() && resp.action != action) {
        qWarning() << "UiController: response action mismatch. Expected:"
                   << action << "got:" << resp.action;
    }

    if (!resp.ok) {
        QString errMsg = resp.error.isEmpty()
                ? QStringLiteral("Daemon responded with status=error for action '%1'").arg(action)
                : resp.error;
        emit controllerError(errMsg);
        return;
    }

    processUiResponse(resp);
}

void UiController::onRequestFailed(const QString &action, const QString &error)
{
    emit controllerError(
        QStringLiteral("Request '%1' failed: %2").arg(action, error)
    );
}

// ----------------------------------------
// Internal: process successful UiResponse
// ----------------------------------------

void UiController::processUiResponse(const UiResponse &resp)
{
    const QString action = resp.action;

    if (action == QStringLiteral("ui_get_status")) {
        UIStatusPayload status;
        QString err;
        if (!JsonProtocol::decodeStatusPayload(resp.payload, status, &err)) {
            emit controllerError(
                QStringLiteral("Failed to decode status payload: %1").arg(err));
            return;
        }
        emit statusUpdated(status);
    }
    else if (action == QStringLiteral("ui_get_stats")) {
        UIStatsPayload stats;
        QString err;
        if (!JsonProtocol::decodeStatsPayload(resp.payload, stats, &err)) {
            emit controllerError(
                QStringLiteral("Failed to decode stats payload: %1").arg(err));
            return;
        }
        emit statsUpdated(stats);
    }
    else if (action == QStringLiteral("ui_get_flows")) {
        QVector<FlowSnapshot> flows;
        QString err;
        if (!JsonProtocol::decodeFlowsPayload(resp.payload, flows, &err)) {
            emit controllerError(
                QStringLiteral("Failed to decode flows payload: %1").arg(err));
            // Even if err is set, decoder is flexible and may still produce flows
        }
        emit flowsUpdated(flows);
    }
    else {
        // Unknown action; not fatal, but log internally
        qWarning() << "UiController: received ui_response for unknown action:" << action;
    }
}

// ----------------------------------------
// Internal slots: PUB/SUB handling
// ----------------------------------------

void UiController::onEventReceived(const QByteArray &json)
{
    DaemonEvent evt;
    QString error;

    if (!JsonProtocol::parseDaemonEvent(json, evt, &error)) {
        emit controllerError(QStringLiteral("Failed to parse daemon_event: %1").arg(error));
        return;
    }

    processDaemonEvent(evt);
}

void UiController::onSubscriberError(const QString &error)
{
    emit controllerError(QStringLiteral("Event subscriber error: %1").arg(error));
}

// ----------------------------------------
// Internal: process DaemonEvent
// ----------------------------------------

void UiController::processDaemonEvent(const DaemonEvent &evt)
{
    const QString ev = evt.event;

    if (ev == QStringLiteral("daemon_alert")) {
        emit daemonAlertReceived(evt);
    }
    else if (ev == QStringLiteral("daemon_error")) {
        emit daemonErrorReceived(evt);
    }
    else if (ev == QStringLiteral("daemon_model_update")) {
        emit daemonModelUpdateReceived(evt);
    }
    else if (ev == QStringLiteral("daemon_status_change")) {
        emit daemonStatusChangeReceived(evt);
    }
    else {
        // Unknown event; not fatal
        qWarning() << "UiController: unknown daemon event:" << ev;
    }
}
