#pragma once

#include <string>

class UILauncher
{
public:
    // Launches the UI executable.
    // If minimized = true → UI starts minimized in the taskbar.
    // If UI is already running → does nothing (optional behavior later).
    static bool launchUI(const std::string& uiPath, bool minimized = true);
};
