#pragma once
#include <string>
#include <mutex>
#include "SystemStats.h"

class StateManager
{
public:
    static StateManager& instance();

    // -------------------------
    // Packet capture state
    // -------------------------
    void setPacketCaptureActive(bool active);
    bool isPacketCaptureActive() const;

    // -------------------------
    // Python ML server state
    // -------------------------
    void setPythonServerOnline(bool online);
    bool isPythonServerOnline() const;

    // -------------------------
    // Last system stats
    // -------------------------
    void setLastSystemStats(const SystemStatsData& stats);
    SystemStatsData getLastSystemStats() const;

    // -------------------------
    // Error tracking (last error only)
    // -------------------------
    void setLastError(const std::string& error);
    std::string getLastError() const;

    // -------------------------
    // Model update versioning
    // -------------------------
    void setModelVersion(const std::string& version);
    std::string getModelVersion() const;

private:
    StateManager() = default;
    ~StateManager() = default;

    StateManager(const StateManager&) = delete;
    StateManager& operator=(const StateManager&) = delete;

private:
    mutable std::mutex m_mutex;

    bool m_packetCaptureActive = false;
    bool m_pythonServerOnline = false;

    SystemStatsData m_lastStats;
    std::string m_lastError;
    std::string m_modelVersion;
};
