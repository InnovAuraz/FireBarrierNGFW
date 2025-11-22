#include "SystemStats.h"
#include <windows.h>
#include <iphlpapi.h>
#include <chrono>

#pragma comment(lib, "iphlpapi.lib")

SystemStats& SystemStats::instance()
{
    static SystemStats inst;
    return inst;
}

SystemStats::SystemStats()
{
    startTimeMs = GetTickCount64();
}

void SystemStats::initialize()
{
    // Initialize CPU baseline values
    FILETIME idleFT, kernelFT, userFT;
    GetSystemTimes(&idleFT, &kernelFT, &userFT);

    ULARGE_INTEGER idle, kernel, user;
    idle.LowPart   = idleFT.dwLowDateTime;
    idle.HighPart  = idleFT.dwHighDateTime;
    kernel.LowPart = kernelFT.dwLowDateTime;
    kernel.HighPart= kernelFT.dwHighDateTime;
    user.LowPart   = userFT.dwLowDateTime;
    user.HighPart  = userFT.dwHighDateTime;

    lastIdleTime   = idle.QuadPart;
    lastKernelTime = kernel.QuadPart;
    lastUserTime   = user.QuadPart;
}

SystemStatsData SystemStats::collect()
{
    SystemStatsData data;

    // 1. CPU usage
    data.cpuUsagePercent = queryCpuUsage();

    // 2. RAM usage
    data.ramUsagePercent = queryRamUsage();

    // 3. Uptime
    data.uptimeMs = GetTickCount64() - startTimeMs;

    // 4. Network counters
    queryNetworkCounters(data.bytesSent, data.bytesReceived);

    return data;
}

// ------------------------------------------------------------
// CPU USAGE
// ------------------------------------------------------------
double SystemStats::queryCpuUsage()
{
    FILETIME idleFT, kernelFT, userFT;
    if (!GetSystemTimes(&idleFT, &kernelFT, &userFT))
        return 0.0;

    ULARGE_INTEGER idle, kernel, user;
    idle.LowPart   = idleFT.dwLowDateTime;
    idle.HighPart  = idleFT.dwHighDateTime;

    kernel.LowPart = kernelFT.dwLowDateTime;
    kernel.HighPart= kernelFT.dwHighDateTime;

    user.LowPart   = userFT.dwLowDateTime;
    user.HighPart  = userFT.dwHighDateTime;

    ULONGLONG idleDiff   = idle.QuadPart   - lastIdleTime;
    ULONGLONG kernelDiff = kernel.QuadPart - lastKernelTime;
    ULONGLONG userDiff   = user.QuadPart   - lastUserTime;

    ULONGLONG total = kernelDiff + userDiff;

    // Update last values
    lastIdleTime   = idle.QuadPart;
    lastKernelTime = kernel.QuadPart;
    lastUserTime   = user.QuadPart;

    if (total == 0)
        return 0.0;

    double usage = (double)(total - idleDiff) / (double)total * 100.0;
    if (usage < 0.0) usage = 0.0;
    if (usage > 100.0) usage = 100.0;

    return usage;
}

// ------------------------------------------------------------
// RAM USAGE
// ------------------------------------------------------------
double SystemStats::queryRamUsage()
{
    MEMORYSTATUSEX mem = {0};
    mem.dwLength = sizeof(mem);

    if (!GlobalMemoryStatusEx(&mem))
        return 0.0;

    DWORDLONG total = mem.ullTotalPhys;
    DWORDLONG free  = mem.ullAvailPhys;

    if (total == 0) return 0.0;

    double used = (double)(total - free) / (double)total * 100.0;
    return used;
}

// ------------------------------------------------------------
// NETWORK COUNTERS (cumulative bytes)
// ------------------------------------------------------------
void SystemStats::queryNetworkCounters(uint64_t& sent, uint64_t& received)
{
    sent = 0;
    received = 0;

    DWORD size = 0;

    // Query buffer size
    if (GetIfTable(nullptr, &size, FALSE) == ERROR_INSUFFICIENT_BUFFER)
    {
        PMIB_IFTABLE table = (PMIB_IFTABLE)malloc(size);
        if (!table)
            return;

        if (GetIfTable(table, &size, FALSE) == NO_ERROR)
        {
            for (DWORD i = 0; i < table->dwNumEntries; i++)
            {
                const MIB_IFROW& row = table->table[i];

                // Skip loopback interfaces
                if (row.dwType == IF_TYPE_SOFTWARE_LOOPBACK)
                    continue;

                sent     += row.dwOutOctets;
                received += row.dwInOctets;
            }
        }

        free(table);
    }
}

