#include "UiController.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include <QDebug>
#include <QFile>
#include <QFileInfo>

static bool daemonIsRunningByPid(const QString &daemonPath)
{
    QString pidFile = QFileInfo(daemonPath).absolutePath() + "/daemon.pid";

    QFile f(pidFile);
    if (!f.exists() || !f.open(QIODevice::ReadOnly))
        return false;

    QByteArray data = f.readAll().trimmed();
    if (data.isEmpty())
        return false;

    bool ok = false;
    qint64 pid = data.toLongLong(&ok);
    if (!ok || pid <= 0)
        return false;

#ifdef Q_OS_WIN
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, DWORD(pid));
    if (!h)
        return false;

    DWORD exitCode=0;
    BOOL alive=GetExitCodeProcess(h,&exitCode);
    CloseHandle(h);
    return alive && exitCode == STILL_ACTIVE;
#else
    return (::kill(pid, 0) == 0);
#endif
}


// ----------------------------------------
// Constructor / Destructor
// ----------------------------------------

UiController::UiController(QObject *parent)
    : QObject(parent),
      m_launcher(this),
      m_client(this)
{
    //
    // Connect DaemonClient signals (PAIR0 channel)
    //
    connect(&m_client, &DaemonClient::uiResponseReceived,
            this, &UiController::onUiResponseReceived);

    connect(&m_client, &DaemonClient::daemonEventReceived,
            this, &UiController::onDaemonEventReceived);

    connect(&m_client, &DaemonClient::connectionError,
            this, &UiController::onConnectionError);

    connect(&m_client, &DaemonClient::connectionEstablished,
            this, []() {
                qDebug() << "[UiController] DaemonClient connected.";
            });
}

UiController::~UiController()
{
    // DaemonClient and DaemonLauncher clean themselves.
    // We intentionally do NOT kill the daemon on UI shutdown.
}

// ----------------------------------------
// Initialization
// ----------------------------------------

bool UiController::initialize()
{
    UiConfig config;
    QString errorMsg;

    if (!UiConfigLoader::load(config, &errorMsg)) {
        emit controllerError(
            QStringLiteral("Failed to load ui_config.json: %1").arg(errorMsg));
        return false;
    }

    m_daemonPath = config.daemonPath;
    m_launcher.setDaemonPath(m_daemonPath);

    // Configure the DaemonClient endpoint (PAIR0 on 6001)
    m_client.setEndpoint(m_reqEndpoint);

    if (daemonIsRunningByPid(m_daemonPath)) {
        qDebug() << "[UiController] Daemon already running → Auto-connect UI client";
        m_client.startClient();      // <--- this triggers delayed connect
    }

    return true;
}

// ----------------------------------------
// Daemon control
// ----------------------------------------

bool UiController::startDaemon()
{
    if (m_daemonPath.isEmpty()) {
        emit controllerError("Daemon path not configured");
        return false;
    }

    //
    // 1. If daemon already running → just start the DaemonClient
    //
    if (daemonIsRunningByPid(m_daemonPath)) {
        qDebug() << "[UiController] Daemon already running, starting client...";
        m_client.startClient();     // <--- NEW
        emit daemonStarted();
        return true;
    }

    //
    // 2. Daemon not running → launch it
    //
    if (!m_launcher.startDaemon()) {
        emit controllerError(
            QStringLiteral("Failed to start daemon at: %1").arg(m_daemonPath));
        return false;
    }

    //
    // 3. NOW start the client (delayed connect)
    //
    qDebug() << "[UiController] Daemon launched, starting client...";
    m_client.startClient();         // <--- NEW

    emit daemonStarted();
    return true;
}


bool UiController::stopDaemon()
{
    if (!m_launcher.isRunning()) {
        emit daemonStopped();
        return true;
    }

    if (!m_launcher.stopDaemon()) {
        emit controllerError(QStringLiteral("Failed to stop daemon"));
        return false;
    }

    emit daemonStopped();
    return true;
}

// ----------------------------------------
// UI request actions (async via DaemonClient)
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
// Slots for DaemonClient signals
// ----------------------------------------

void UiController::onUiResponseReceived(const UiResponse &resp)
{
    processUiResponse(resp);
}

void UiController::onDaemonEventReceived(const DaemonEvent &evt)
{
    processDaemonEvent(evt);
}

void UiController::onConnectionError(const QString &error)
{
    emit controllerError(error);
}

// ----------------------------------------
// Internal: process successful UiResponse
// ----------------------------------------

void UiController::processUiResponse(const UiResponse &resp)
{
    const QString action = resp.action;

    if (!resp.ok) {
        const QString msg = resp.error.isEmpty()
            ? QStringLiteral("Daemon responded with status=error for action '%1'")
                  .arg(action)
            : resp.error;
        emit controllerError(msg);
        return;
    }

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
            // decoder may still have produced some flows
        }
        emit flowsUpdated(flows);
    }
    else {
        // Unknown action; not fatal, but log internally
        qWarning() << "UiController: received ui_response for unknown action:" << action;
    }
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
