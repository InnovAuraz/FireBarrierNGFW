#include "Config.h"
#include "Logger.h"

#include <fstream>
#include <sstream>
#include <mutex>
#include "json.hpp"

using json = nlohmann::json;

static std::mutex g_configMutex;

Config& Config::instance()
{
    static Config inst;
    return inst;
}

bool Config::load(const std::string& path)
{
    std::lock_guard<std::mutex> lock(g_configMutex);

    std::ifstream file(path);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open config file: " + path);
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    json j;
    try
    {
        j = json::parse(buffer.str());
    }
    catch (const std::exception& ex)
    {
        LOG_ERROR(std::string("Failed to parse config JSON: ") + ex.what());
        return false;
    }

    // Read fields with defaults if missing
    if (j.contains("log_file"))               m_logFile = j["log_file"].get<std::string>();
    if (j.contains("nng_server_address"))     m_nngServerAddress = j["nng_server_address"].get<std::string>();
    if (j.contains("nng_client_address"))     m_nngClientAddress = j["nng_client_address"].get<std::string>();
    if (j.contains("pcap_device"))            m_pcapDevice = j["pcap_device"].get<std::string>();
    if (j.contains("stats_interval_ms"))      m_statsIntervalMs = j["stats_interval_ms"].get<int>();
    if (j.contains("python_server_timeout_ms")) m_pythonServerTimeoutMs = j["python_server_timeout_ms"].get<int>();
    m_uiExeName = j.value("ui_exe_name", "FireBarrier.exe");

    LOG_INFO("Config loaded successfully from " + path);
    return true;
}

// ----------------------- GETTERS ----------------------------

const std::string& Config::logFile() const               { return m_logFile; }
const std::string& Config::nngServerAddress() const      { return m_nngServerAddress; }
const std::string& Config::nngClientAddress() const      { return m_nngClientAddress; }
const std::string& Config::pcapDevice() const            { return m_pcapDevice; }
const std::string& Config::uiExeName() const             { return m_uiExeName; }

int Config::statsIntervalMs() const                      { return m_statsIntervalMs; }
int Config::pythonServerTimeoutMs() const                { return m_pythonServerTimeoutMs; }
