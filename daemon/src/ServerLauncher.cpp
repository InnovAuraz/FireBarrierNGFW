#include "ServerLauncher.h"
#include "Logger.h"
#include <windows.h>

bool ServerLauncher::launchPythonServer(const std::string& exeDir)
{
    // Expected layout:
    // FireBarrier/
    //   daemon.exe
    //   python_runtime/
    //       python.exe
    //       main.py
    std::string pythonExe = exeDir + "\\python_runtime\\python.exe";
    std::string script    = exeDir + "\\python_runtime\\main.py";

    LOG_INFO("Launching Python server: " + pythonExe + " " + script);

    std::wstring wPythonExe(pythonExe.begin(), pythonExe.end());
    std::wstring wCmd =
        L"\"" + wPythonExe + L"\" \"" +
        std::wstring(script.begin(), script.end()) + L"\"";

    STARTUPINFOW si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);

    BOOL ok = CreateProcessW(
        wPythonExe.c_str(),
        &wCmd[0],
        nullptr, nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr,
        std::wstring(exeDir.begin(), exeDir.end()).c_str(),
        &si, &pi
    );

    if (!ok)
    {
        DWORD err = GetLastError();
        LOG_ERROR("ServerLauncher: Failed to start Python server (err=" + std::to_string(err) + ")");
        return false;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    LOG_INFO("ServerLauncher: Python server started successfully.");
    return true;
}
