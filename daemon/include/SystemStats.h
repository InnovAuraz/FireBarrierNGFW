#pragma once
#include <cstdint>

struct SystemStatsData
{
    double cpuUsagePercent = 0.0;   // 0–100
    double ramUsagePercent = 0.0;   // 0–100

    uint64_t uptimeMs = 0;

    uint64_t bytesSent = 0;         // cumulative
    uint64_t bytesReceived = 0;     // cumulative
};

class SystemStats
{
public:
    static SystemStats& instance();

    // Must be called once during daemon startup
    void initialize();

    // Collect stats (DaemonController will call this every stats_interval_ms)
    SystemStatsData collect();

private:
    SystemStats();

    // Internal helpers (OS-specific — Windows)
    double queryCpuUsage();
    double queryRamUsage();

    void queryNetworkCounters(uint64_t& sent, uint64_t& received);

private:
    uint64_t startTimeMs = 0;

    // CPU usage tracking
    uint64_t lastIdleTime = 0;
    uint64_t lastKernelTime = 0;
    uint64_t lastUserTime = 0;
};
