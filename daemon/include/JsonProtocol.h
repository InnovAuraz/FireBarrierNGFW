#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

#include "FlowBuilder.h"

// -----------------------------
// Daemon → Python: flow_analysis_request
// -----------------------------

struct FlowTransportStats
{
    std::vector<int>    packetSizes;      // "packet_sizes"
    std::vector<int>    directions;       // "directions"
    std::vector<double> interArrivalMs;   // "inter_arrival_ms"
    uint64_t            totalBytes   = 0; // "total_bytes"
    uint32_t            numPackets   = 0; // "num_packets"
};

struct FlowTlsInfo
{
    bool        isTls       = false;      // "is_tls"
    std::string version;                 // "version"
    std::string sni;                     // "sni"
    std::string cipherSuite;             // "cipher_suite"
};

struct FlowStatsInfo
{
    double entropy        = 0.0;         // "entropy"
    double durationMs     = 0.0;         // "duration_ms"
    double meanPacketSize = 0.0;         // "mean_packet_size"
};

struct FlowInfo
{
    std::string flowId;                  // "flow_id"
    std::string srcIp;                   // "src_ip"
    std::string dstIp;                   // "dst_ip"
    uint16_t    srcPort   = 0;           // "src_port"
    uint16_t    dstPort   = 0;           // "dst_port"
    std::string protocol;                // "protocol" (e.g., "TCP")

    FlowTransportStats transport;        // "transport"
    FlowTlsInfo        tls;             // "tls"
    FlowStatsInfo      stats;           // "stats"
};

struct FlowAnalysisRequestData
{
    std::string version  = "1.0";        // "version"
    std::string type     = "flow_analysis_request"; // "type"
    uint64_t    timestamp = 0;           // "timestamp"
    std::string hostId;                  // "host_id"

    FlowInfo    flow;                    // "flow"
};

// -----------------------------
// Python → Daemon: flow_analysis_response
// -----------------------------

struct FlowAnalysisResult
{
    bool        valid          = false;

    std::string version;                 // "version"
    std::string type;                    // "type"
    uint64_t    timestamp      = 0;      // "timestamp"

    std::string flowId;                  // "flow_id"

    std::string classification;          // "classification"
    double      threatScore    = 0.0;    // "threat_score"
    double      riskScore      = 0.0;    // "risk_score"
    std::string zeroTrustDecision;       // "zero_trust_decision"

    std::string category;                // "category"
    double      confidence     = 0.0;    // "confidence"

    // explanation
    std::string modelVersion;            // explanation.model_version
    int         federatedRound = 0;      // explanation.federated_round

    // explanation.features_used.*
    double      featureEntropy      = 0.0;
    std::string featurePacketPattern;
    std::string featureTlsVersion;
    std::string featureSni;

    std::string notes;                  // explanation.notes
};

// -----------------------------
// Daemon → Python: daemon_status_ping
// -----------------------------

struct DaemonStatusPingData
{
    std::string version  = "1.0";        // "version"
    std::string type     = "daemon_status_ping"; // "type"
    uint64_t    timestamp = 0;           // "timestamp"
    std::string hostId;                  // "host_id"

    double      cpuUsage       = 0.0;    // status.cpu_usage
    double      memUsage       = 0.0;    // status.mem_usage
    uint64_t    flowsProcessed = 0;      // status.flows_processed
    uint64_t    uptimeSec      = 0;      // status.uptime_sec
};

// -----------------------------
// Python → Daemon: ack
// -----------------------------

struct PythonAck
{
    std::string version;                 // "version"
    std::string type;                    // "type" == "ack"
    uint64_t    timestamp    = 0;        // "timestamp"
    std::string message;                 // "message"
    bool        valid        = false;
};

// -----------------------------
// Python → Daemon: error
// -----------------------------

struct PythonErrorEvent
{
    std::string version;                 // "version"
    std::string type;                    // "type" == "error"
    uint64_t    timestamp = 0;           // "timestamp"

    std::string code;                    // error.code
    std::string message;                 // error.message
    std::string details;                 // error.details

    bool        valid    = false;
};

// -----------------------------
// Python → Daemon: model_update
// -----------------------------

struct ModelUpdatePush
{
    std::string version;                 // "version"
    std::string type;                    // "type" == "model_update"
    uint64_t    timestamp = 0;           // "timestamp"

    std::string modelVersion;            // "model_version"
    int         federatedRound = 0;      // "federated_round"

    std::string compression;             // update.compression
    std::string format;                  // update.format
    std::string payloadBase64;           // update.payload

    bool        valid = false;
};

// ===========================================================
// UI ↔ Daemon structures (data-only UI protocol)
// ===========================================================

struct UiRequest
{
    std::string action;       // "ui_get_status", "ui_get_stats", "ui_get_flows"
    std::string type;         // "ui_request"
    std::string version;      // "1.0"
    std::string rawJson;      // original JSON (optional)
};

struct UiResponse
{
    std::string action;       // echo of request action
    bool ok = false;          // "ok" or "error"
    std::string error;        // error message if any
    std::string payloadJson;  // JSON-encoded object containing the data
};

struct DaemonEvent
{
    std::string event;         // "daemon_alert", "daemon_error", "daemon_model_update", "daemon_status_change"
    std::string payloadJson;   // JSON object as string
};

class JsonProtocol
{
public:
    static FlowAnalysisRequestData convertFlowRecordToRequest(const FlowRecord& flow, const std::string& hostId);

    // Daemon → Python: flow_analysis_request
    static std::string buildFlowAnalysisRequest(const FlowAnalysisRequestData& req);

    // Daemon → Python: daemon_status_ping
    static std::string buildDaemonStatusPing(const DaemonStatusPingData& ping);

    // Python → Daemon: flow_analysis_response
    static std::optional<FlowAnalysisResult>
        parseFlowAnalysisResponse(const std::string& jsonStr);

    // Python → Daemon: ack
    static std::optional<PythonAck>
        parsePythonAck(const std::string& jsonStr);

    // Python → Daemon: error
    static std::optional<PythonErrorEvent>
        parsePythonError(const std::string& jsonStr);

    // Python → Daemon: model_update
    static std::optional<ModelUpdatePush>
        parseModelUpdatePush(const std::string& jsonStr);
    
    // ---- UI → Daemon: parse ui_request ----
    static std::optional<UiRequest>
        parseUiRequest(const std::string& jsonStr);

    // ---- Daemon → UI: build ui_response ----
    static std::string
        buildUiResponse(const UiResponse& resp);

    // ---- Daemon → UI: build daemon_event ----
    static std::string
        buildDaemonEvent(const DaemonEvent& evt);

    // ---- Helper: build status payload for UI ----
    static std::string buildUiStatusPayload(
        bool daemonAlive,
        bool pythonOnline,
        bool captureRunning,
        uint64_t uptimeSec,
        uint64_t flowsProcessed,
        const std::string& modelVersion,
        const std::string& lastError
    );

    // ---- Helper: build stats payload ----
    static std::string buildUiStatsPayload(
        double cpuUsage,
        double memUsage,
        double networkSent,
        double networkReceived
    );

    // ---- Helper: build flows payload ----
    static std::string buildUiFlowsPayload(
        const std::vector<FlowRecord>& flows
    );
};
