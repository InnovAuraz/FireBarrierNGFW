#include "UILauncher.h"
#include "Logger.h"

#include <windows.h>
#include <string>

static std::wstring getDaemonDirectory()
{
    wchar_t buffer[MAX_PATH];
    DWORD len = GetModuleFileNameW(NULL, buffer, MAX_PATH);
    if (len == 0)
        return L".";

    std::wstring path(buffer);
    size_t pos = path.find_last_of(L"\\/");
    if (pos != std::wstring::npos)
        path = path.substr(0, pos);

    return path;
}

static std::wstring getUiPidFile()
{
    std::wstring dir = getDaemonDirectory();
    return dir + L"\\ui.pid";
}

static bool writeUiPid(DWORD pid)
{
    std::wstring path = getUiPidFile();
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (h == INVALID_HANDLE_VALUE)
        return false;

    std::string s = std::to_string(pid);
    DWORD written = 0;
    WriteFile(h, s.c_str(), (DWORD)s.size(), &written, nullptr);
    CloseHandle(h);
    return true;
}

static bool readUiPid(DWORD &pidOut)
{
    std::wstring path = getUiPidFile();
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (h == INVALID_HANDLE_VALUE)
        return false;

    char buf[32] = {0};
    DWORD read = 0;
    ReadFile(h, buf, sizeof(buf)-1, &read, nullptr);
    CloseHandle(h);

    if (read == 0)
        return false;

    pidOut = std::strtoul(buf, nullptr, 10);
    return (pidOut > 0);
}

static bool isUiRunning()
{
    DWORD pid = 0;
    if (!readUiPid(pid))
        return false;

    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h)
        return false;

    DWORD exitCode = 0;
    BOOL ok = GetExitCodeProcess(h, &exitCode);
    CloseHandle(h);

    return (ok && exitCode == STILL_ACTIVE);
}

// Convert UTF-8 → UTF-16
static std::wstring toWide(const std::string& s)
{
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring wide(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &wide[0], sizeNeeded);
    return wide;
}

bool UILauncher::launchUI(const std::string& uiName, bool minimized)
{
    // -------------------------------
    // 1) Prevent duplicate launches
    // -------------------------------
    if (isUiRunning()) {
        LOG_INFO("UI already running — skipping launch.");
        return true;
    }

    std::wstring daemonDir = getDaemonDirectory();
    std::wstring uiPath = daemonDir + L"\\" + toWide(uiName);

    LOG_INFO("Launching UI: " + uiName);

    STARTUPINFOW si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    si.cb = sizeof(si);

    if (minimized) {
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_SHOWMINIMIZED;
    }

    BOOL ok = CreateProcessW(
        uiPath.c_str(),
        nullptr,
        nullptr, nullptr,
        FALSE,
        0,
        nullptr,
        daemonDir.c_str(),
        &si,
        &pi
    );

    if (!ok) {
        DWORD err = GetLastError();
        LOG_ERROR("Failed to launch UI. Error code=" + std::to_string(err));
        return false;
    }

    // ------------------------------------
    // 2) Save UI PID for next time
    // ------------------------------------
    writeUiPid(pi.dwProcessId);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    LOG_INFO("UI launched successfully. PID saved.");
    return true;
}
