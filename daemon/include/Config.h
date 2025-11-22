#pragma once
#include <string>

class Config
{
public:
    static Config& instance();

    // Load values from JSON file (called once during daemon startup)
    bool load(const std::string& path);

    // Accessors
    const std::string& logFile() const;
    const std::string& nngServerAddress() const;
    const std::string& nngClientAddress() const;
    const std::string& pcapDevice() const;
    const std::string& uiExeName() const;

    int statsIntervalMs() const;
    int pythonServerTimeoutMs() const;

private:
    Config() = default;
    ~Config() = default;

    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

private:
    std::string m_logFile;
    std::string m_nngServerAddress;
    std::string m_nngClientAddress;
    std::string m_pcapDevice;
    std::string m_uiExeName;

    int m_statsIntervalMs = 1000;
    int m_pythonServerTimeoutMs = 5000;
};
