#pragma once

#include <string>
#include <thread>
#include <atomic>

#include "SystemStats.h"
#include "StateManager.h"
#include "PacketCapture.h"
#include "FlowBuilder.h"
#include "NngServer.h"
#include "NngClient.h"
#include "JsonProtocol.h"
#include "UILauncher.h"
#include "Config.h"
#include "ServerLauncher.h"

// DaemonController is the central coordinator for:
// - Packet capture + flow building
// - UI NNG server
// - Python NNG client
// - System stats + heartbeats
// - StateManager updates
// - UI alerts launching
class DaemonController
{
public:
    DaemonController();
    ~DaemonController();

    // Load config, initialize members (but do not start threads yet)
    bool initialize(const std::string& configPath);

    // Start all subsystems (NNG, capture, heartbeat)
    bool start();

    // Stop everything and join threads; safe to call from destructor
    void stop();

    // Blocking run loop – keeps daemon alive until stop() is requested
    // (For now this can just sleep/poll on m_running in a loop.)
    void run();

private:
    DaemonController(const DaemonController&) = delete;
    DaemonController& operator=(const DaemonController&) = delete;

    // ----- Setup helpers -----
    bool setupUiServer();
    bool setupPythonClient();
    bool setupPacketCapture();
    bool startHeartbeatThread();

    // ----- NNG message handlers -----
    // Called by NngServer when UI sends a message
    void onUiMessage(const std::string& json);

    // Called by NngClient when Python server sends a message
    void onPythonMessage(const std::string& json);

    // ----- UI request handling (data only) -----
    void handleUiRequest(const UiRequest& req);

    void handleUiGetStatus(const std::string& action);
    void handleUiGetStats(const std::string& action);
    void handleUiGetFlows(const std::string& action);

    // ----- Python message handling -----
    void handleFlowAnalysisResponse(const FlowAnalysisResult& result);
    void handlePythonError(const PythonErrorEvent& errorEvent);
    void handleModelUpdate(const ModelUpdatePush& update);
    void handlePythonAck(const PythonAck& ack);

    // ----- Heartbeat / periodic tasks -----
    void heartbeatLoop();  // sends daemon_status_ping to Python periodically

    // ----- Packet capture integration -----
    void onPacketCaptured(const CapturedPacket& pkt);

    // ----- UI alert integration -----
    void sendUiAlertEvent(const std::string& title,
                          const std::string& message,
                          const std::string& severity);

    void ensureUiLaunchedForAlert(const std::string& reason);

    // Try to start the Python backend process (python_runtime/python.exe ...).
    bool launchPythonServerProcess();

    // Try connecting to Python NNG endpoint, with limited retries.
    bool ensurePythonServerConnected(int maxAttempts);

    // ----- Flow → Python submission -----
    void sendFlowToPython(const FlowRecord& flow);

private:
    // Core components
    Config&        m_config;
    StateManager&  m_state;
    SystemStats&   m_systemStats;
    PacketCapture  m_packetCapture;
    FlowBuilder    m_flowBuilder;
    NngServer      m_uiServer;
    NngClient      m_pythonClient;

    // Heartbeat / control
    std::thread        m_heartbeatThread;
    std::atomic<bool>  m_running{false};

    // Addresses / IDs from config
    std::string m_uiAddress;       // e.g. "ipc://ui_daemon.ipc" or "tcp://127.0.0.1:6001"
    std::string m_pythonAddress;   // e.g. "tcp://127.0.0.1:7001"
    std::string m_hostId;          // host UUID used in JSON pings
    std::string m_uiExeName;       // e.g. "FireBarrierUI.exe"
};
