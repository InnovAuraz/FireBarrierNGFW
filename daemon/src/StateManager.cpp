#include "StateManager.h"

StateManager& StateManager::instance()
{
    static StateManager inst;
    return inst;
}

// -------------------------------------------------------------
// Packet capture state
// -------------------------------------------------------------
void StateManager::setPacketCaptureActive(bool active)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_packetCaptureActive = active;
}

bool StateManager::isPacketCaptureActive() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_packetCaptureActive;
}

// -------------------------------------------------------------
// Python server connectivity state
// -------------------------------------------------------------
void StateManager::setPythonServerOnline(bool online)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pythonServerOnline = online;
}

bool StateManager::isPythonServerOnline() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pythonServerOnline;
}

// -------------------------------------------------------------
// Last SystemStats snapshot
// -------------------------------------------------------------
void StateManager::setLastSystemStats(const SystemStatsData& stats)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastStats = stats;
}

SystemStatsData StateManager::getLastSystemStats() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastStats;
}

// -------------------------------------------------------------
// Error tracking
// -------------------------------------------------------------
void StateManager::setLastError(const std::string& error)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastError = error;
}

std::string StateManager::getLastError() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastError;
}

// -------------------------------------------------------------
// Model update versioning
// -------------------------------------------------------------
void StateManager::setModelVersion(const std::string& version)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_modelVersion = version;
}

std::string StateManager::getModelVersion() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_modelVersion;
}
