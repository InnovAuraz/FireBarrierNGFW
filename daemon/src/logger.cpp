#include "Logger.h"
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>
namespace fs = std::filesystem;


static std::mutex g_logMutex;

Logger& Logger::instance()
{
    static Logger inst;
    return inst;
}

void Logger::initialize(const std::string& filePath)
{
    std::lock_guard<std::mutex> lock(g_logMutex);
    logFile = filePath;
    initialized = true;
}

static const char* levelToString(LogLevel level)
{
    switch (level)
    {
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
        default:              return "UNKNOWN";
    }
}

void Logger::log(LogLevel level,
                 const std::string& message,
                 const char* file,
                 int line)
{
    if (!initialized) return;

    std::lock_guard<std::mutex> lock(g_logMutex);

    rotateLogsIfNeeded();

    std::ofstream out(logFile, std::ios::app);
    if (!out.is_open()) return;

    // Timestamp
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);

    std::tm tm_buf;
    localtime_s(&tm_buf, &t);

    char timeBuf[32];
    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tm_buf);

    out << "[" << timeBuf << "] "
        << "[" << levelToString(level) << "] "
        << "(" << file << ":" << line << ") "
        << message
        << std::endl;
}

uint64_t Logger::getFileSize(const std::string& path)
{
    if (!fs::exists(path))
        return 0;
    return fs::file_size(path);
}

void Logger::rotateLogsIfNeeded()
{
    if (!initialized)
        return;

    if (getFileSize(logFile) < MAX_LOG_SIZE)
        return;

    // Rotate: daemon.log.2 → daemon.log.3
    for (int i = MAX_BACKUPS; i >= 1; --i)
    {
        std::string oldName = logFile + "." + std::to_string(i);
        std::string newName = logFile + "." + std::to_string(i + 1);

        if (fs::exists(oldName))
        {
            // If exceeding max backups, delete
            if (i == MAX_BACKUPS)
                fs::remove(oldName);
            else
                fs::rename(oldName, newName);
        }
    }

    // Rotate current log → daemon.log.1
    std::string firstBackup = logFile + ".1";
    if (fs::exists(logFile))
        fs::rename(logFile, firstBackup);
}
