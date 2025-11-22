#include "DaemonController.h"
#include "Logger.h"

#include "json.hpp"
using json = nlohmann::json;

#include <chrono>
#include <thread>

// For JsonProtocol we already included its header in DaemonController.h

using namespace std::chrono_literals;

DaemonController::DaemonController()
    : m_config(Config::instance())
    , m_state(StateManager::instance())
    , m_systemStats(SystemStats::instance())
{
}

DaemonController::~DaemonController()
{
    stop();
}

bool DaemonController::initialize(const std::string& configPath)
{
    LOG_INFO("DaemonController::initialize - loading config from: " + configPath);

    if (!m_config.load(configPath))
    {
        LOG_ERROR("Failed to load config file.");
        return false;
    }

    m_systemStats.initialize();

    // These keys are assumptions; adjust if your Config uses different names.
    m_uiAddress     = m_config.nngServerAddress();
    m_pythonAddress = m_config.nngClientAddress();

    // These two aren’t in Config yet; use safe defaults for now:
    m_hostId    = "edge-node-uuid-unknown";
    m_uiExeName = m_config.uiExeName();

    LOG_INFO("Config: ui_address=" + m_uiAddress +
             ", python_address=" + m_pythonAddress +
             ", host_id=" + m_hostId +
             ", ui_exe_name=" + m_uiExeName);

    // Set packet capture callback; actual start is done in setupPacketCapture/start()
    m_packetCapture.setPacketHandler(
        [this](const CapturedPacket& pkt)
        {
            this->onPacketCaptured(pkt);
        }
    );

    // When a flow finalizes, send it to Python backend
    m_flowBuilder.setFlowReadyCallback(
        [this](const FlowRecord& flow)
        {
            this->sendFlowToPython(flow);
        }
    );

    return true;
}

bool DaemonController::start()
{
    LOG_INFO("DaemonController::start");

    m_running = true;

    if (!setupUiServer())
    {
        LOG_ERROR("Failed to start UI NNG server.");
        m_running = false;
        return false;
    }

    if (!setupPythonClient())
    {
        LOG_ERROR("Failed to start Python NNG client.");
        m_running = false;
        return false;
    }

    if (!setupPacketCapture())
    {
        LOG_ERROR("Packet capture setup failed; daemon cannot continue without capture.");

        sendUiAlertEvent(
            "Packet capture failure",
            "The daemon could not start packet capture on the configured device. "
            "Firewall functionality is disabled.",
            "high"
        );

        ensureUiLaunchedForAlert("pcap_failure");

        m_running = false;
        return false;
    }

    if (!startHeartbeatThread())
    {
        LOG_ERROR("Failed to start heartbeat thread.");
        m_running = false;
        return false;
    }

    LOG_INFO("DaemonController started successfully.");
    return true;
}

void DaemonController::stop()
{
    if (!m_running.exchange(false))
        return; // already stopped

    LOG_INFO("DaemonController::stop - stopping subsystems");

    // Stop heartbeat thread
    if (m_heartbeatThread.joinable())
    {
        m_heartbeatThread.join();
    }

    // Stop packet capture
    m_packetCapture.stop();

    // Stop NNG endpoints
    m_uiServer.stop();
    m_pythonClient.stop();

    LOG_INFO("DaemonController::stop - all subsystems stopped.");
}

void DaemonController::run()
{
    LOG_INFO("DaemonController::run - entering main loop");

    while (m_running)
    {
        std::this_thread::sleep_for(500ms);
        // In the future you can add housekeeping here if needed.
    }

    LOG_INFO("DaemonController::run - exiting main loop");
}

// ----------------------------------------------------
// Setup helpers
// ----------------------------------------------------

bool DaemonController::setupUiServer()
{
    LOG_INFO("Setting up UI NNG server at: " + m_uiAddress);

    m_uiServer.setMessageHandler(
        [this](const std::string& msg)
        {
            this->onUiMessage(msg);
        }
    );

    if (!m_uiServer.start(m_uiAddress))
    {
        LOG_ERROR("NngServer failed to start at: " + m_uiAddress);
        return false;
    }

    LOG_INFO("UI NNG server started successfully.");
    return true;
}

bool DaemonController::setupPythonClient()
{
    LOG_INFO("Setting up Python NNG client at: " + m_pythonAddress);

    m_pythonClient.setMessageHandler(
        [this](const std::string& msg)
        {
            this->onPythonMessage(msg);
        }
    );

    // Retry a few times and auto-launch Python if needed
    const int maxAttempts = 3;
    if (!ensurePythonServerConnected(maxAttempts))
    {
        LOG_ERROR("Python NNG client setup failed after retries.");
        return false;
    }

    LOG_INFO("Python NNG client is connected and ready.");
    return true;
}

bool DaemonController::setupPacketCapture()
{
    LOG_INFO("Starting packet capture (auto-select device)");

    if (!m_packetCapture.startAuto())
    {
        LOG_ERROR("Packet capture failed to start on any device.");
        return false;
    }

    LOG_INFO("Packet capture started.");
    return true;
}

bool DaemonController::startHeartbeatThread()
{
    int hbIntervalMs = m_config.statsIntervalMs();
    if (hbIntervalMs <= 0) hbIntervalMs = 1000;

    LOG_INFO("Starting heartbeat thread, interval (ms) = " + std::to_string(hbIntervalMs));

    m_heartbeatThread = std::thread(
        [this, hbIntervalMs]()
        {
            this->heartbeatLoop();
        }
    );

    return true;
}

// ----------------------------------------------------
// UI message handling
// ----------------------------------------------------

void DaemonController::onUiMessage(const std::string& json)
{
    LOG_INFO("Received UI message: " + json);

    auto maybeReq = JsonProtocol::parseUiRequest(json);
    if (!maybeReq.has_value())
    {
        LOG_ERROR("Failed to parse UI request JSON.");
        // Optionally send an error response back.
        UiResponse resp;
        resp.action      = "";
        resp.ok          = false;
        resp.error       = "invalid_request";
        resp.payloadJson = "{}";
        m_uiServer.send(JsonProtocol::buildUiResponse(resp));
        return;
    }

    handleUiRequest(*maybeReq);
}

void DaemonController::handleUiRequest(const UiRequest& req)
{
    LOG_INFO("Handling UI request: action=" + req.action);

    if (req.action == "ui_get_status")
    {
        handleUiGetStatus(req.action);
    }
    else if (req.action == "ui_get_stats")
    {
        handleUiGetStats(req.action);
    }
    else if (req.action == "ui_get_flows")
    {
        handleUiGetFlows(req.action);
    }
    else
    {
        LOG_WARN("Unknown UI action: " + req.action);
        UiResponse resp;
        resp.action      = req.action;
        resp.ok          = false;
        resp.error       = "unknown_action";
        resp.payloadJson = "{}";
        m_uiServer.send(JsonProtocol::buildUiResponse(resp));
    }
}

void DaemonController::handleUiGetStatus(const std::string& action)
{
    bool daemonAlive    = m_running.load();
    bool pythonOnline   = m_pythonClient.isConnected();
    bool captureRunning = m_packetCapture.isRunning(); // implement isRunning() if not present

    // TODO: track real uptime and flows processed via StateManager
    uint64_t uptimeSec      = 0;
    uint64_t flowsProcessed = 0;
    std::string modelVersion; // get from StateManager later
    std::string lastError;    // get from StateManager later

    std::string payload = JsonProtocol::buildUiStatusPayload(
        daemonAlive,
        pythonOnline,
        captureRunning,
        uptimeSec,
        flowsProcessed,
        modelVersion,
        lastError
    );

    UiResponse resp;
    resp.action      = action;
    resp.ok          = true;
    resp.payloadJson = payload;

    m_uiServer.send(JsonProtocol::buildUiResponse(resp));
}

void DaemonController::handleUiGetStats(const std::string& action)
{
    SystemStatsData s = m_systemStats.collect();

    std::string payload = JsonProtocol::buildUiStatsPayload(
        s.cpuUsagePercent,
        s.ramUsagePercent,
        static_cast<double>(s.bytesSent),
        static_cast<double>(s.bytesReceived)
    );

    UiResponse resp;
    resp.action      = action;
    resp.ok          = true;
    resp.payloadJson = payload;

    m_uiServer.send(JsonProtocol::buildUiResponse(resp));
}

void DaemonController::handleUiGetFlows(const std::string& action)
{
    auto flows    = m_flowBuilder.getFlowsSnapshot();
    std::string payload = JsonProtocol::buildUiFlowsPayload(flows);

    UiResponse resp;
    resp.action      = action;
    resp.ok          = true;
    resp.payloadJson = payload;

    m_uiServer.send(JsonProtocol::buildUiResponse(resp));
}

// ----------------------------------------------------
// Python message handling
// ----------------------------------------------------

void DaemonController::onPythonMessage(const std::string& json)
{
    LOG_INFO("Received Python message: " + json);

    // Try to parse in order of most-specific
    if (auto res = JsonProtocol::parseFlowAnalysisResponse(json))
    {
        handleFlowAnalysisResponse(*res);
        return;
    }

    if (auto err = JsonProtocol::parsePythonError(json))
    {
        handlePythonError(*err);
        return;
    }

    if (auto mu = JsonProtocol::parseModelUpdatePush(json))
    {
        handleModelUpdate(*mu);
        return;
    }

    if (auto ack = JsonProtocol::parsePythonAck(json))
    {
        handlePythonAck(*ack);
        return;
    }

    LOG_WARN("Unknown Python message format/type.");
}

void DaemonController::handleFlowAnalysisResponse(const FlowAnalysisResult& result)
{
    LOG_INFO("FlowAnalysisResult: flow_id=" + result.flowId +
             ", classification=" + result.classification +
             ", decision=" + result.zeroTrustDecision);

    // Example: if malicious/quarantine → alert UI
    if (result.classification == "malicious" ||
        result.zeroTrustDecision == "quarantine")
    {
        std::string title   = "Threat Detected";
        std::string message = "Flow " + result.flowId +
                              " classified as " + result.classification +
                              " (decision=" + result.zeroTrustDecision + ")";
        std::string severity = "high";

        sendUiAlertEvent(title, message, severity);
    }

    // TODO: update StateManager with metrics, last incident, etc.
}

void DaemonController::handlePythonError(const PythonErrorEvent& errorEvent)
{
    LOG_ERROR("PythonErrorEvent: code=" + errorEvent.code +
              ", msg=" + errorEvent.message +
              ", details=" + errorEvent.details);

    // TODO: update StateManager lastError

    // Also inform UI
    DaemonEvent evt;
    json payload;
    payload["code"]    = errorEvent.code;
    payload["message"] = errorEvent.message;
    payload["details"] = errorEvent.details;

    evt.event       = "daemon_error";
    evt.payloadJson = payload.dump();

    m_uiServer.send(JsonProtocol::buildDaemonEvent(evt));

    // Optionally ensure UI is launched for visibility
    ensureUiLaunchedForAlert("python_error");
}

void DaemonController::handleModelUpdate(const ModelUpdatePush& update)
{
    LOG_INFO("ModelUpdate: version=" + update.modelVersion +
             ", round=" + std::to_string(update.federatedRound));

    // TODO: Store model update to disk, notify python worker, etc.

    // Notify UI (only metadata, not payload)
    DaemonEvent evt;
    json payload;
    payload["model_version"]   = update.modelVersion;
    payload["federated_round"] = update.federatedRound;
    payload["compression"]     = update.compression;
    payload["format"]          = update.format;

    evt.event       = "model_update";
    evt.payloadJson = payload.dump();

    m_uiServer.send(JsonProtocol::buildDaemonEvent(evt));
}

void DaemonController::handlePythonAck(const PythonAck& ack)
{
    LOG_INFO("Python ACK received: message=" + ack.message);
    // You can track last heartbeat ack time via StateManager if needed.
}

void DaemonController::sendFlowToPython(const FlowRecord& flow)
{
    LOG_INFO("Preparing flow for Python submission.");

    // Convert FlowRecord → FlowAnalysisRequestData
    FlowAnalysisRequestData req =
        JsonProtocol::convertFlowRecordToRequest(flow, m_hostId);

    // Serialize JSON
    std::string jsonMsg = JsonProtocol::buildFlowAnalysisRequest(req);

    // Ensure Python client is live
    if (!m_pythonClient.isConnected())
    {
        LOG_WARN("Python client not connected — attempting reconnect.");

        if (!ensurePythonServerConnected(3))
        {
            LOG_ERROR("Failed to reconnect to Python server — flow dropped.");
            return;
        }
    }

    // Send
    if (!m_pythonClient.send(jsonMsg))
    {
        LOG_ERROR("Failed to send flow_analysis_request to Python.");
        return;
    }

    LOG_INFO("Flow successfully submitted to Python.");
}

// ----------------------------------------------------
// Heartbeat / periodic ping
// ----------------------------------------------------

void DaemonController::heartbeatLoop()
{
    LOG_INFO("Heartbeat thread started.");

    int hbIntervalMs = m_config.statsIntervalMs();
    if (hbIntervalMs <= 0) hbIntervalMs = 1000;

    while (m_running)
    {
        // 1) Collect system stats
        SystemStatsData s = m_systemStats.collect();

        // 2) Expire inactive flows and send them to Python
        uint64_t nowMs =
            static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count());

        m_flowBuilder.checkForExpiredFlows(nowMs);

        // 3) Build and send daemon_status_ping
        DaemonStatusPingData ping;
        ping.version   = "1.0";
        ping.type      = "daemon_status_ping";
        ping.timestamp = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count());
        ping.hostId    = m_hostId;

        ping.cpuUsage       = s.cpuUsagePercent;
        ping.memUsage       = s.ramUsagePercent;
        ping.flowsProcessed = 0;                  // TODO: hook from StateManager
        ping.uptimeSec      = s.uptimeMs / 1000;  // ms → sec

        std::string json = JsonProtocol::buildDaemonStatusPing(ping);

        if (!m_pythonClient.send(json))
        {
            LOG_WARN("Failed to send daemon_status_ping to Python.");
        }

        // 4) Sleep until next heartbeat
        int slept = 0;
        while (slept < hbIntervalMs && m_running)
        {
            std::this_thread::sleep_for(100ms);
            slept += 100;
        }
    }

    LOG_INFO("Heartbeat thread exiting.");
}

// ----------------------------------------------------
// Packet capture integration
// ----------------------------------------------------

void DaemonController::onPacketCaptured(const CapturedPacket& pkt)
{
    // Feed to FlowBuilder
    m_flowBuilder.processPacket(pkt);
}

// ----------------------------------------------------
// UI alert helpers
// ----------------------------------------------------

void DaemonController::sendUiAlertEvent(const std::string& title,
                                        const std::string& message,
                                        const std::string& severity)
{
    DaemonEvent evt;
    json payload;
    payload["title"]    = title;
    payload["message"]  = message;
    payload["severity"] = severity;

    evt.event       = "daemon_alert";
    evt.payloadJson = payload.dump();

    m_uiServer.send(JsonProtocol::buildDaemonEvent(evt));

    ensureUiLaunchedForAlert("alert_event");
}

void DaemonController::ensureUiLaunchedForAlert(const std::string& reason)
{
    LOG_INFO("Ensuring UI is launched for alert, reason=" + reason);

    // For now we *always* attempt to launch UI when a serious alert occurs.
    // In the future, you can track if UI is already connected via NngServer.
    UILauncher::launchUI(m_uiExeName, true);
}

bool DaemonController::launchPythonServerProcess()
{
    LOG_INFO("DaemonController: attempting to launch Python backend...");

    // For now, current working directory is used:
    // FireBarrier/
    //   daemon.exe
    //   ui.exe
    //   python_runtime/
    std::string exeDir = ".";

    if (!ServerLauncher::launchPythonServer(exeDir))
    {
        LOG_ERROR("DaemonController: failed to launch Python backend process.");
        return false;
    }

    LOG_INFO("DaemonController: Python backend launch requested successfully.");
    return true;
}

bool DaemonController::ensurePythonServerConnected(int maxAttempts)
{
    int timeoutMs = m_config.pythonServerTimeoutMs();
    if (timeoutMs <= 0) timeoutMs = 5000;

    for (int attempt = 1; attempt <= maxAttempts && m_running; ++attempt)
    {
        LOG_INFO("DaemonController: Python connect attempt " +
                 std::to_string(attempt) + " / " + std::to_string(maxAttempts));

        // Launch Python if not running
        launchPythonServerProcess();

        // Attempt NNG connection
        if (m_pythonClient.start(m_pythonAddress))
        {
            LOG_INFO("DaemonController: connected to Python backend at " + m_pythonAddress);
            m_state.setPythonServerOnline(true);
            return true;
        }

        LOG_WARN("DaemonController: cannot connect to Python at " + m_pythonAddress +
                 ", retrying in " + std::to_string(timeoutMs) + " ms...");

        m_state.setPythonServerOnline(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(timeoutMs));
    }

    LOG_ERROR("DaemonController: Python backend unavailable after retries.");
    sendUiAlertEvent(
        "Python backend unavailable",
        "Daemon could not connect to the Python ML backend.",
        "high"
    );
    ensureUiLaunchedForAlert("python_unavailable");

    return false;
}
