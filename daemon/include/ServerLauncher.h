#pragma once
#include <string>

class ServerLauncher
{
public:
    // Launch the Python backend process.
    // exeDir = directory of daemon.exe
    static bool launchPythonServer(const std::string& exeDir);
};
