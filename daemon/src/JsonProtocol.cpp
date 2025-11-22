#include "JsonProtocol.h"
#include "Logger.h"

#include "json.hpp"

using json = nlohmann::json;

// -----------------------------
// Helpers
// -----------------------------

static uint64_t get_uint64_safe(const json& j, const char* key, uint64_t def = 0)
{
    if (!j.contains(key)) return def;
    if (!j[key].is_number()) return def;
    return j[key].get<uint64_t>();
}

static double get_double_safe(const json& j, const char* key, double def = 0.0)
{
    if (!j.contains(key)) return def;
    if (!j[key].is_number()) return def;
    return j[key].get<double>();
}

static std::string get_string_safe(const json& j, const char* key, const std::string& def = {})
{
    if (!j.contains(key)) return def;
    if (!j[key].is_string()) return def;
    return j[key].get<std::string>();
}

// -----------------------------
// Daemon → Python: buildFlowAnalysisRequest
// -----------------------------

std::string JsonProtocol::buildFlowAnalysisRequest(
    const FlowAnalysisRequestData& req)
{
    json j;

    j["version"]  = req.version;
    j["type"]     = req.type;
    j["timestamp"] = req.timestamp;
    j["host_id"]  = req.hostId;

    const FlowInfo& f = req.flow;

    json jf;
    jf["flow_id"]  = f.flowId;
    jf["src_ip"]   = f.srcIp;
    jf["dst_ip"]   = f.dstIp;
    jf["src_port"] = f.srcPort;
    jf["dst_port"] = f.dstPort;
    jf["protocol"] = f.protocol;

    // transport
    json jt;
    jt["packet_sizes"]     = f.transport.packetSizes;
    jt["directions"]       = f.transport.directions;
    jt["inter_arrival_ms"] = f.transport.interArrivalMs;
    jt["total_bytes"]      = f.transport.totalBytes;
    jt["num_packets"]      = f.transport.numPackets;

    // tls
    json jtls;
    jtls["is_tls"]       = f.tls.isTls;
    jtls["version"]      = f.tls.version;
    jtls["sni"]          = f.tls.sni;
    jtls["cipher_suite"] = f.tls.cipherSuite;

    // stats
    json js;
    js["entropy"]          = f.stats.entropy;
    js["duration_ms"]      = f.stats.durationMs;
    js["mean_packet_size"] = f.stats.meanPacketSize;

    jf["transport"] = jt;
    jf["tls"]       = jtls;
    jf["stats"]     = js;

    j["flow"] = jf;

    return j.dump();
}

// -----------------------------
// Daemon → Python: buildDaemonStatusPing
// -----------------------------

std::string JsonProtocol::buildDaemonStatusPing(const DaemonStatusPingData& ping)
{
    json j;
    j["version"]   = ping.version;
    j["type"]      = ping.type;
    j["timestamp"] = ping.timestamp;
    j["host_id"]   = ping.hostId;

    json st;
    st["cpu_usage"]       = ping.cpuUsage;
    st["mem_usage"]       = ping.memUsage;
    st["flows_processed"] = ping.flowsProcessed;
    st["uptime_sec"]      = ping.uptimeSec;

    j["status"] = st;

    return j.dump();
}

// -----------------------------
// Python → Daemon: flow_analysis_response
// -----------------------------

std::optional<FlowAnalysisResult>
JsonProtocol::parseFlowAnalysisResponse(const std::string& jsonStr)
{
    json j;
    try
    {
        j = json::parse(jsonStr);
    }
    catch (const std::exception& ex)
    {
        LOG_ERROR(std::string("Failed to parse flow_analysis_response JSON: ") + ex.what());
        return std::nullopt;
    }

    if (!j.contains("type") || !j["type"].is_string())
        return std::nullopt;
    std::string type = j["type"].get<std::string>();
    if (type != "flow_analysis_response")
        return std::nullopt;

    FlowAnalysisResult res;
    res.type      = type;
    res.version   = get_string_safe(j, "version", "1.0");
    res.timestamp = get_uint64_safe(j, "timestamp", 0);

    res.flowId         = get_string_safe(j, "flow_id");
    res.classification = get_string_safe(j, "classification");
    res.threatScore    = get_double_safe(j, "threat_score", 0.0);
    res.riskScore      = get_double_safe(j, "risk_score", 0.0);
    res.zeroTrustDecision = get_string_safe(j, "zero_trust_decision");
    res.category       = get_string_safe(j, "category");
    res.confidence     = get_double_safe(j, "confidence", 0.0);

    if (j.contains("explanation") && j["explanation"].is_object())
    {
        const json& e = j["explanation"];
        res.modelVersion   = get_string_safe(e, "model_version");
        res.federatedRound = static_cast<int>(get_uint64_safe(e, "federated_round", 0));

        if (e.contains("features_used") && e["features_used"].is_object())
        {
            const json& f = e["features_used"];
            res.featureEntropy       = get_double_safe(f, "entropy", 0.0);
            res.featurePacketPattern = get_string_safe(f, "packet_length_pattern");
            res.featureTlsVersion    = get_string_safe(f, "tls_version");
            res.featureSni           = get_string_safe(f, "sni");
        }

        res.notes = get_string_safe(e, "notes");
    }

    res.valid = true;
    return res;
}

// -----------------------------
// Python → Daemon: ack
// -----------------------------

std::optional<PythonAck>
JsonProtocol::parsePythonAck(const std::string& jsonStr)
{
    json j;
    try
    {
        j = json::parse(jsonStr);
    }
    catch (const std::exception& ex)
    {
        LOG_ERROR(std::string("Failed to parse ack JSON: ") + ex.what());
        return std::nullopt;
    }

    if (!j.contains("type") || !j["type"].is_string())
        return std::nullopt;
    std::string type = j["type"].get<std::string>();
    if (type != "ack")
        return std::nullopt;

    PythonAck ack;
    ack.type      = type;
    ack.version   = get_string_safe(j, "version", "1.0");
    ack.timestamp = get_uint64_safe(j, "timestamp", 0);
    ack.message   = get_string_safe(j, "message");
    ack.valid     = true;

    return ack;
}

// -----------------------------
// Python → Daemon: error
// -----------------------------

std::optional<PythonErrorEvent>
JsonProtocol::parsePythonError(const std::string& jsonStr)
{
    json j;
    try
    {
        j = json::parse(jsonStr);
    }
    catch (const std::exception& ex)
    {
        LOG_ERROR(std::string("Failed to parse error JSON: ") + ex.what());
        return std::nullopt;
    }

    if (!j.contains("type") || !j["type"].is_string())
        return std::nullopt;
    std::string type = j["type"].get<std::string>();
    if (type != "error")
        return std::nullopt;

    PythonErrorEvent ev;
    ev.type      = type;
    ev.version   = get_string_safe(j, "version", "1.0");
    ev.timestamp = get_uint64_safe(j, "timestamp", 0);

    if (j.contains("error") && j["error"].is_object())
    {
        const json& e = j["error"];
        ev.code    = get_string_safe(e, "code");
        ev.message = get_string_safe(e, "message");
        ev.details = get_string_safe(e, "details");
    }

    ev.valid = true;
    return ev;
}

// -----------------------------
// Python → Daemon: model_update
// -----------------------------

std::optional<ModelUpdatePush>
JsonProtocol::parseModelUpdatePush(const std::string& jsonStr)
{
    json j;
    try
    {
        j = json::parse(jsonStr);
    }
    catch (const std::exception& ex)
    {
        LOG_ERROR(std::string("Failed to parse model_update JSON: ") + ex.what());
        return std::nullopt;
    }

    if (!j.contains("type") || !j["type"].is_string())
        return std::nullopt;
    std::string type = j["type"].get<std::string>();
    if (type != "model_update")
        return std::nullopt;

    ModelUpdatePush mu;
    mu.type        = type;
    mu.version     = get_string_safe(j, "version", "1.0");
    mu.timestamp   = get_uint64_safe(j, "timestamp", 0);
    mu.modelVersion= get_string_safe(j, "model_version");
    mu.federatedRound = static_cast<int>(get_uint64_safe(j, "federated_round", 0));

    if (j.contains("update") && j["update"].is_object())
    {
        const json& u = j["update"];
        mu.compression   = get_string_safe(u, "compression");
        mu.format        = get_string_safe(u, "format");
        mu.payloadBase64 = get_string_safe(u, "payload");
    }

    mu.valid = true;
    return mu;
}

// -----------------------------------------------------------
// Parse UI → Daemon request
// -----------------------------------------------------------

std::optional<UiRequest>
JsonProtocol::parseUiRequest(const std::string& jsonStr)
{
    json j;
    try {
        j = json::parse(jsonStr);
    } catch (...) {
        LOG_ERROR("Invalid UI request JSON");
        return std::nullopt;
    }

    if (!j.contains("type") || j["type"] != "ui_request")
        return std::nullopt;

    UiRequest req;
    req.type    = "ui_request";
    req.version = j.value("version", "1.0");
    req.action  = j.value("action", "");
    req.rawJson = jsonStr;

    if (req.action.empty())
        return std::nullopt;

    return req;
}

// -----------------------------------------------------------
// Build Daemon → UI response
// -----------------------------------------------------------

std::string JsonProtocol::buildUiResponse(const UiResponse& resp)
{
    json j;
    j["type"]   = "ui_response";
    j["version"]= "1.0";
    j["action"] = resp.action;
    j["status"] = resp.ok ? "ok" : "error";

    if (!resp.error.empty())
        j["error"] = resp.error;

    if (!resp.payloadJson.empty()) {
        try {
            j["payload"] = json::parse(resp.payloadJson);
        } catch (...) {
            j["payload"] = resp.payloadJson; // fallback
        }
    }

    return j.dump();
}

// -----------------------------------------------------------
// Build Daemon → UI event
// -----------------------------------------------------------

std::string JsonProtocol::buildDaemonEvent(const DaemonEvent& evt)
{
    json j;
    j["type"]    = "daemon_event";
    j["version"] = "1.0";
    j["event"]   = evt.event;

    if (!evt.payloadJson.empty()) {
        try {
            j["payload"] = json::parse(evt.payloadJson);
        } catch (...) {
            j["payload"] = evt.payloadJson;
        }
    }

    return j.dump();
}

// -----------------------------------------------------------
// Build UI status payload
// -----------------------------------------------------------

std::string JsonProtocol::buildUiStatusPayload(
    bool daemonAlive,
    bool pythonOnline,
    bool captureRunning,
    uint64_t uptimeSec,
    uint64_t flowsProcessed,
    const std::string& modelVersion,
    const std::string& lastError
)
{
    json p;
    p["daemon_alive"]     = daemonAlive;
    p["python_online"]    = pythonOnline;
    p["capture_running"]  = captureRunning;
    p["uptime_sec"]       = uptimeSec;
    p["flows_processed"]  = flowsProcessed;
    p["model_version"]    = modelVersion;
    p["last_error"]       = lastError;

    return p.dump();
}

// -----------------------------------------------------------
// Build UI stats payload
// -----------------------------------------------------------

std::string JsonProtocol::buildUiStatsPayload(
    double cpuUsage,
    double memUsage,
    double networkSent,
    double networkReceived
)
{
    json p;
    p["cpu_usage"]       = cpuUsage;
    p["mem_usage"]       = memUsage;
    p["network_sent"]    = networkSent;
    p["network_received"]= networkReceived;

    return p.dump();
}

// -----------------------------------------------------------
// Build flows snapshot payload
// -----------------------------------------------------------

std::string JsonProtocol::buildUiFlowsPayload(
    const std::vector<FlowRecord>& flows
)
{
    json arr = json::array();

    for (const auto& f : flows) {
        json j;
        j["src_ip"]    = f.key.srcIp;
        j["dst_ip"]    = f.key.dstIp;
        j["src_port"]  = f.key.srcPort;
        j["dst_port"]  = f.key.dstPort;
        j["protocol"]  = f.key.protocol;

        j["first_seen_ms"] = f.firstSeenMs;
        j["last_seen_ms"]  = f.lastSeenMs;

        j["bytes_forward"] = f.bytesForward;
        j["bytes_reverse"] = f.bytesReverse;

        j["packets_forward"] = f.packetCountForward;
        j["packets_reverse"] = f.packetCountReverse;

        arr.push_back(j);
    }

    return arr.dump();
}

static std::string ipv4ToString(uint32_t ip)
{
    return std::to_string((ip >> 24) & 0xFF) + "." +
           std::to_string((ip >> 16) & 0xFF) + "." +
           std::to_string((ip >> 8)  & 0xFF) + "." +
           std::to_string(ip & 0xFF);
}


FlowAnalysisRequestData
JsonProtocol::convertFlowRecordToRequest(
    const FlowRecord& f, const std::string& hostId)
{
    FlowAnalysisRequestData req;

    req.version   = "1.0";
    req.type      = "flow_analysis_request";
    req.timestamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()
    );
    req.hostId    = hostId;

    // -------------------------------
    // FlowInfo
    // -------------------------------
    FlowInfo& info = req.flow;

    // Build a stable flow_id: srcIp-dstIp-srcPort-dstPort-proto
    info.flowId  = 
        ipv4ToString(f.key.srcIp) + "-" +
        ipv4ToString(f.key.dstIp) + "-" +
        std::to_string(f.key.srcPort) + "-" +
        std::to_string(f.key.dstPort) + "-" +
        std::to_string(f.key.protocol);

    info.srcIp   = ipv4ToString(f.key.srcIp);
    info.dstIp   = ipv4ToString(f.key.dstIp);
    info.srcPort = f.key.srcPort;
    info.dstPort = f.key.dstPort;
    info.protocol = (f.key.protocol == 6 ? "TCP" :
                    f.key.protocol == 17 ? "UDP" : "OTHER");

    // -------------------------------
    // Transport stats
    // -------------------------------
    info.transport.packetSizes = 
        std::vector<int>(f.packetSizes.begin(), f.packetSizes.end());

    info.transport.directions =
        std::vector<int>(f.directions.begin(), f.directions.end());

    info.transport.interArrivalMs =
        std::vector<double>(f.interArrivalMs.begin(), f.interArrivalMs.end());

    info.transport.totalBytes =
        f.bytesForward + f.bytesReverse;

    info.transport.numPackets =
        f.packetCountForward + f.packetCountReverse;

    // -------------------------------
    // TLS info
    // -------------------------------
    info.tls.isTls       = f.isTls;
    info.tls.version     = f.tlsVersion;
    info.tls.sni         = f.sni;
    info.tls.cipherSuite = f.cipherSuite;

    // -------------------------------
    // Stats
    // -------------------------------
    info.stats.entropy        = f.entropy;
    info.stats.durationMs     = (double)f.durationMs;
    info.stats.meanPacketSize = f.meanPacketSize;

    return req;
}
