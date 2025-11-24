#include "JsonProtocol.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

// ===========================================================
// Internal helper functions (Qt-side)
// ===========================================================

namespace {

// Get object[key] as bool; fallback if missing or wrong type
bool getBool(const QJsonObject &obj, const char *key, bool def = false)
{
    auto it = obj.find(QString::fromUtf8(key));
    if (it == obj.end() || (!it->isBool() && !it->isDouble() && !it->isString()))
        return def;

    if (it->isBool())
        return it->toBool();

    if (it->isDouble())
        return it->toDouble() != 0.0;

    if (it->isString()) {
        const QString s = it->toString().trimmed().toLower();
        if (s == "true" || s == "1" || s == "yes")
            return true;
        if (s == "false" || s == "0" || s == "no")
            return false;
    }

    return def;
}

// Get object[key] as 64-bit unsigned; fallback if missing or wrong type
quint64 getUint64(const QJsonObject &obj, const char *key, quint64 def = 0)
{
    auto it = obj.find(QString::fromUtf8(key));
    if (it == obj.end() || (!it->isDouble() && !it->isString()))
        return def;

    if (it->isDouble()) {
        double d = it->toDouble();
        if (d < 0.0)
            return def;
        return static_cast<quint64>(d);
    }

    bool ok = false;
    quint64 v = it->toString().toULongLong(&ok);
    return ok ? v : def;
}

// Get object[key] as double; fallback if missing or wrong type
double getDouble(const QJsonObject &obj, const char *key, double def = 0.0)
{
    auto it = obj.find(QString::fromUtf8(key));
    if (it == obj.end() || (!it->isDouble() && !it->isString()))
        return def;

    if (it->isDouble())
        return it->toDouble();

    bool ok = false;
    double v = it->toString().toDouble(&ok);
    return ok ? v : def;
}

// Get object[key] as QString; fallback if missing or wrong type
QString getString(const QJsonObject &obj, const char *key, const QString &def = QString())
{
    auto it = obj.find(QString::fromUtf8(key));
    if (it == obj.end() || !it->isString())
        return def;

    return it->toString();
}

} // namespace

// ===========================================================
// UI → Daemon: build ui_request
// ===========================================================

QByteArray JsonProtocol::buildUiRequest(const QString &action)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("type"),    QStringLiteral("ui_request"));
    obj.insert(QStringLiteral("version"), QStringLiteral("1.0"));
    obj.insert(QStringLiteral("action"),  action);

    QJsonDocument doc(obj);
    return doc.toJson(QJsonDocument::Compact);
}

// ===========================================================
// Daemon → UI: parse ui_response
// ===========================================================

bool JsonProtocol::parseUiResponse(const QByteArray &jsonData,
                                   UiResponse &out,
                                   QString *errorMessage)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Invalid JSON in ui_response: %1")
                                .arg(parseError.errorString());
        }
        return false;
    }

    QJsonObject obj = doc.object();

    // type must be "ui_response"
    QString type = getString(obj, "type");
    if (type != QStringLiteral("ui_response")) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Invalid type for ui_response: '%1'").arg(type);
        }
        return false;
    }

    out.type    = type;
    out.version = getString(obj, "version", QStringLiteral("1.0"));
    out.action  = getString(obj, "action");

    // status: "ok" or "error"
    QString status = getString(obj, "status", QStringLiteral("error")).toLower();
    out.ok = (status == QStringLiteral("ok"));

    // error (optional)
    out.error.clear();
    if (obj.contains(QStringLiteral("error")) && obj.value(QStringLiteral("error")).isString()) {
        out.error = obj.value(QStringLiteral("error")).toString();
    }

    // payload (optional, any JSON type)
    if (obj.contains(QStringLiteral("payload"))) {
        out.payload = obj.value(QStringLiteral("payload"));
    } else {
        out.payload = QJsonValue(QJsonValue::Null);
    }

    return true;
}

// ===========================================================
// Daemon → UI: parse daemon_event
// ===========================================================

bool JsonProtocol::parseDaemonEvent(const QByteArray &jsonData,
                                    DaemonEvent &out,
                                    QString *errorMessage)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Invalid JSON in daemon_event: %1")
                                .arg(parseError.errorString());
        }
        return false;
    }

    QJsonObject obj = doc.object();

    QString type = getString(obj, "type");
    if (type != QStringLiteral("daemon_event")) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Invalid type for daemon_event: '%1'").arg(type);
        }
        return false;
    }

    out.type    = type;
    out.version = getString(obj, "version", QStringLiteral("1.0"));
    out.event   = getString(obj, "event");

    if (obj.contains(QStringLiteral("payload"))) {
        out.payload = obj.value(QStringLiteral("payload"));
    } else {
        out.payload = QJsonValue(QJsonValue::Null);
    }

    return true;
}

// ===========================================================
// Helpers: decode status payload
// ===========================================================

bool JsonProtocol::decodeStatusPayload(const QJsonValue &payload,
                                       UIStatusPayload &out,
                                       QString *errorMessage)
{
    if (!payload.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Status payload is not a JSON object");
        }
        return false;
    }

    QJsonObject obj = payload.toObject();

    // Flexible parsing: missing fields use defaults instead of failing
    out.daemonAlive     = getBool(obj, "daemon_alive", false);
    out.pythonOnline    = getBool(obj, "python_online", false);
    out.captureRunning  = getBool(obj, "capture_running", false);
    out.uptimeSec       = getUint64(obj, "uptime_sec", 0);
    out.flowsProcessed  = getUint64(obj, "flows_processed", 0);
    out.modelVersion    = getString(obj, "model_version", QString());
    out.lastError       = getString(obj, "last_error", QString());

    Q_UNUSED(errorMessage);
    return true;
}

// ===========================================================
// Helpers: decode stats payload
// ===========================================================

bool JsonProtocol::decodeStatsPayload(const QJsonValue &payload,
                                      UIStatsPayload &out,
                                      QString *errorMessage)
{
    if (!payload.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Stats payload is not a JSON object");
        }
        return false;
    }

    QJsonObject obj = payload.toObject();

    out.cpuUsage        = getDouble(obj, "cpu_usage", 0.0);
    out.memUsage        = getDouble(obj, "mem_usage", 0.0);
    out.networkSent     = getDouble(obj, "network_sent", 0.0);
    out.networkReceived = getDouble(obj, "network_received", 0.0);

    Q_UNUSED(errorMessage);
    return true;
}

// ===========================================================
// Helpers: decode flows payload (array of FlowSnapshot)
// ===========================================================

bool JsonProtocol::decodeFlowsPayload(const QJsonValue &payload,
                                      QVector<FlowSnapshot> &out,
                                      QString *errorMessage)
{
    if (!payload.isArray()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Flows payload is not a JSON array");
        }
        return false;
    }

    QJsonArray arr = payload.toArray();
    out.clear();
    out.reserve(arr.size());

    for (const QJsonValue &v : arr) {
        if (!v.isObject()) {
            // Skip invalid entries but keep going (flexible parsing)
            continue;
        }

        QJsonObject obj = v.toObject();
        FlowSnapshot f;

        f.srcIp   = getString(obj, "src_ip");
        f.dstIp   = getString(obj, "dst_ip");
        f.srcPort = static_cast<quint16>(getUint64(obj, "src_port", 0));
        f.dstPort = static_cast<quint16>(getUint64(obj, "dst_port", 0));
        f.protocol = getString(obj, "protocol");

        f.firstSeenMs    = getUint64(obj, "first_seen_ms", 0);
        f.lastSeenMs     = getUint64(obj, "last_seen_ms", 0);
        f.bytesForward   = getUint64(obj, "bytes_forward", 0);
        f.bytesReverse   = getUint64(obj, "bytes_reverse", 0);
        f.packetsForward = getUint64(obj, "packets_forward", 0);
        f.packetsReverse = getUint64(obj, "packets_reverse", 0);

        out.push_back(f);
    }

    // Even if array had some bad entries, we keep the good ones.
    if (out.isEmpty() && arr.size() > 0) {
        // Everything was invalid – optionally report
        if (errorMessage) {
            *errorMessage = QStringLiteral("No valid flow entries found in flows payload");
        }
        // Still return true, because the structure (array) was correct.
        return true;
    }

    return true;
}
