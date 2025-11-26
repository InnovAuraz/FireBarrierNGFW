#include "DaemonClient.h"
#include "JsonProtocol.h"
#include "DaemonClientWorker.h"

#include "UiDebugLog.h"

#include <QMetaObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

extern "C" {
#include <nng/nng.h>
#include <nng/protocol/pair0/pair.h>
}

// -----------------------------------------------------------------------------
// DaemonClient
// -----------------------------------------------------------------------------

DaemonClient::DaemonClient(QObject *parent)
    : QObject(parent)
{
    ui_debug_log("DaemonClient: constructor");
    
    m_running = false;   // IMPORTANT: do NOT start yet
}

DaemonClient::~DaemonClient()
{
    ui_debug_log("DaemonClient: destructor called");
    m_running = false;

    if (m_worker) {
        // 🔥 Direct stop → no queued connection, no event loop needed
        m_worker->requestStopFromOwner();
    }

    // This will wait until ioLoop exits and the thread finishes
    m_ioThread.quit();
    m_ioThread.wait();

    delete m_worker;
    m_worker = nullptr;

    ui_debug_log("DaemonClient: destructor finished");
}

// -----------------------------------------------------------------------------
// Start / Stop Client (NEW)
// -----------------------------------------------------------------------------

bool DaemonClient::startClient()
{
    if (m_running)
        return true;

    m_running = true;

    // Create worker now
    m_worker = new DaemonClientWorker(m_endpoint);
    m_worker->moveToThread(&m_ioThread);

    // Worker lifecycle
    connect(&m_ioThread, &QThread::started,
            m_worker, &DaemonClientWorker::start);

    connect(&m_ioThread, &QThread::started,
            m_worker, &DaemonClientWorker::ioLoop);

    connect(&m_ioThread, &QThread::finished,
            m_worker, &DaemonClientWorker::stop);

    // Relay worker→client signals
    connect(m_worker, &DaemonClientWorker::connectionEstablished,
            this, &DaemonClient::connectionEstablished);

    connect(m_worker, &DaemonClientWorker::connectionError,
            this, &DaemonClient::connectionError);

    connect(m_worker, &DaemonClientWorker::uiResponseReceived,
            this, &DaemonClient::uiResponseReceived);

    connect(m_worker, &DaemonClientWorker::daemonEventReceived,
            this, &DaemonClient::daemonEventReceived);

    // Send JSON to worker
    connect(this, &DaemonClient::sendJson,
            m_worker, &DaemonClientWorker::sendJson,
            Qt::QueuedConnection);

    m_ioThread.start();
    return true;
}

void DaemonClient::stopClient()
{
    if (!m_running)
        return;

    m_running = false;

    m_ioThread.quit();
    m_ioThread.wait();

    delete m_worker;
    m_worker = nullptr;
}

// -----------------------------------------------------------------------------
// Endpoint
// -----------------------------------------------------------------------------

void DaemonClient::setEndpoint(const QString &endpoint)
{
    m_endpoint = endpoint;
}

QString DaemonClient::endpoint() const
{
    return m_endpoint;
}

// -----------------------------------------------------------------------------
// UI Requests
// -----------------------------------------------------------------------------

void DaemonClient::requestStatus()
{
    if (!m_running) return;
    QByteArray json = buildRequestJson("ui_get_status");
    emit sendJson(json);
}

void DaemonClient::requestStats()
{
    if (!m_running) return;
    QByteArray json = buildRequestJson("ui_get_stats");
    emit sendJson(json);
}

void DaemonClient::requestFlows()
{
    if (!m_running) return;
    QByteArray json = buildRequestJson("ui_get_flows");
    emit sendJson(json);
}

// -----------------------------------------------------------------------------
// Build JSON
// -----------------------------------------------------------------------------

QByteArray DaemonClient::buildRequestJson(const QString &action) const
{
    QJsonObject obj;
    obj["type"]    = "ui_request";
    obj["version"] = "1.0";
    obj["action"]  = action;
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}
