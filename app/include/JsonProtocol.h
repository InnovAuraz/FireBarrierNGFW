#pragma once

#include <QString>
#include <QByteArray>
#include <QVector>
#include <QtGlobal>
#include <QJsonObject>
#include <QJsonArray>

// ===========================================================
// UI-side equivalents of daemon structures (Qt-flavoured)
// ===========================================================

// Envelope: Daemon → UI response
struct UiResponse
{
    QString type;          // "ui_response"
    QString version;       // "1.0"
    QString action;        // "ui_get_status" / "ui_get_stats" / "ui_get_flows"
    bool    ok = false;    // status == "ok"
    QString error;         // error message if any
    QJsonValue payload;    // raw payload JSON (object/array/string/null)
};

// Envelope: Daemon → UI event (PUB/SUB)
struct DaemonEvent
{
    QString type;         // "daemon_event"
    QString version;      // "1.0"
    QString event;        // "daemon_alert", "daemon_error", "daemon_model_update", "daemon_status_change"
    QJsonValue payload;   // event-specific payload
};

// Status payload for "ui_get_status"
struct UIStatusPayload
{
    bool daemonAlive     = false;   // "daemon_alive"
    bool pythonOnline    = false;   // "python_online"
    bool captureRunning  = false;   // "capture_running"
    quint64 uptimeSec    = 0;       // "uptime_sec"
    quint64 flowsProcessed = 0;     // "flows_processed"
    QString modelVersion;           // "model_version"
    QString lastError;              // "last_error"
};

// Stats payload for "ui_get_stats"
struct UIStatsPayload
{
    double cpuUsage        = 0.0;   // "cpu_usage"
    double memUsage        = 0.0;   // "mem_usage"
    double networkSent     = 0.0;   // "network_sent"
    double networkReceived = 0.0;   // "network_received"
};

// Flow snapshot record (from buildUiFlowsPayload)
struct FlowSnapshot
{
    QString srcIp;           // "src_ip"
    QString dstIp;           // "dst_ip"
    quint16 srcPort = 0;     // "src_port"
    quint16 dstPort = 0;     // "dst_port"
    QString protocol;        // "protocol"

    quint64 firstSeenMs    = 0;  // "first_seen_ms"
    quint64 lastSeenMs     = 0;  // "last_seen_ms"

    quint64 bytesForward   = 0;  // "bytes_forward"
    quint64 bytesReverse   = 0;  // "bytes_reverse"
    quint64 packetsForward = 0;  // "packets_forward"
    quint64 packetsReverse = 0;  // "packets_reverse"
};


// ===========================================================
// JsonProtocol: Qt-side helpers matching daemon protocol
// ===========================================================

class JsonProtocol
{
public:
    // ---- UI → Daemon: build ui_request ----
    // Must match JsonProtocol::parseUiRequest on daemon side:
    // {
    //   "type": "ui_request",
    //   "version": "1.0",
    //   "action": "<action>"
    // }
    static QByteArray buildUiRequest(const QString &action);

    // ---- Daemon → UI: parse ui_response ----
    // Validates:
    //   type == "ui_response"
    //   version (optional, default "1.0")
    //   action
    //   status == "ok" | "error"
    // Extracts:
    //   error (if any)
    //   payload (as QJsonValue)
    //
    // Returns true on success, false on invalid/parse error.
    static bool parseUiResponse(const QByteArray &jsonData,
                                UiResponse &out,
                                QString *errorMessage = nullptr);

    // ---- Daemon → UI: parse daemon_event ----
    // Validates:
    //   type == "daemon_event"
    //   version (optional, default "1.0")
    //   event
    // Extracts:
    //   payload (as QJsonValue)
    static bool parseDaemonEvent(const QByteArray &jsonData,
                                 DaemonEvent &out,
                                 QString *errorMessage = nullptr);

    // ---- Helpers: decode payloads into typed structs ----

    // For "ui_get_status" payload
    static bool decodeStatusPayload(const QJsonValue &payload,
                                    UIStatusPayload &out,
                                    QString *errorMessage = nullptr);

    // For "ui_get_stats" payload
    static bool decodeStatsPayload(const QJsonValue &payload,
                                   UIStatsPayload &out,
                                   QString *errorMessage = nullptr);

    // For "ui_get_flows" payload
    // payload is expected to be a JSON array
    static bool decodeFlowsPayload(const QJsonValue &payload,
                                   QVector<FlowSnapshot> &out,
                                   QString *errorMessage = nullptr);
};
