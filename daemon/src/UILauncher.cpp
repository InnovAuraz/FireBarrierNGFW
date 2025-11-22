#include "UILauncher.h"
#include "Logger.h"

#include <windows.h>
#include <string>

// Convert UTF-8 → UTF-16
static std::wstring toWide(const std::string& s)
{
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring wide(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &wide[0], sizeNeeded);
    return wide;
}

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

bool UILauncher::launchUI(const std::string& uiName, bool minimized)
{
    std::wstring daemonDir = getDaemonDirectory();
    std::wstring uiPath = daemonDir + L"\\" + toWide(uiName);

    LOG_INFO("Launching UI: " + std::string(uiName));

    STARTUPINFOW si = { 0 };
    PROCESS_INFORMATION pi = { 0 };

    si.cb = sizeof(si);

    if (minimized) {
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_SHOWMINIMIZED;
    }

    BOOL ok = CreateProcessW(
        uiPath.c_str(),    // application path
        nullptr,           // command-line args
        nullptr, nullptr,  // process/thread security
        FALSE,             // inherit handles
        0,                 // creation flags
        nullptr,           // environment
        daemonDir.c_str(), // working directory
        &si, &pi
    );

    if (!ok) {
        DWORD err = GetLastError();
        LOG_ERROR("Failed to launch UI. Error code=" + std::to_string(err));
        return false;
    }

    // We don't need the process handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    LOG_INFO("UI launched successfully.");
    return true;
}
