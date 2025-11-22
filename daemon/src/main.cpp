#include "DaemonController.h"
#include "Logger.h"
#include <iostream>

int main()
{
    // 1. Initialize logger BEFORE anything else
    Logger::instance().initialize("daemon.log");

    std::cout << "Starting FireBarrier daemon..." << std::endl;

    DaemonController daemon;

    // 2. Load config (daemon_config.json must be beside exe)
    if (!daemon.initialize("daemon_config.json")) {
        std::cout << "Daemon initialize() failed." << std::endl;
        return 1;
    }

    // 3. Start subsystems (UI server, Python client, packet capture)
    if (!daemon.start()) {
        std::cout << "Daemon start() failed." << std::endl;
        return 1;
    }

    std::cout << "Daemon running." << std::endl;

    // 4. Block forever (until stop() is called internally)
    daemon.run();

    std::cout << "Daemon exiting." << std::endl;

    return 0;
}
