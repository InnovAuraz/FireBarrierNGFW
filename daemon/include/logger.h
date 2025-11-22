#pragma once
#include <string>

enum class LogLevel {
    Info,
    Warn,
    Error
};

class Logger {
public:
    static Logger& instance();

    void initialize(const std::string& filePath);

    void log(LogLevel level,
             const std::string& message,
             const char* file,
             int line);

private:
    Logger() = default;
    ~Logger() = default;

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string logFile;
    bool initialized = false;

    void rotateLogsIfNeeded();
    uint64_t getFileSize(const std::string& path);

    static constexpr uint64_t MAX_LOG_SIZE = 5ULL * 1024ULL * 1024ULL; // 5 MB
    static constexpr int MAX_BACKUPS = 3;
};

// Convenience macros
#define LOG_INFO(msg)  Logger::instance().log(LogLevel::Info,  (msg), __FILE__, __LINE__)
#define LOG_WARN(msg)  Logger::instance().log(LogLevel::Warn,  (msg), __FILE__, __LINE__)
#define LOG_ERROR(msg) Logger::instance().log(LogLevel::Error, (msg), __FILE__, __LINE__)
