#pragma once
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include "nng/nng.h"
#include <nng/protocol/pair0/pair.h>

class NngServer
{
public:
    NngServer();
    ~NngServer();

    // Bind to an address (from Config)
    bool start(const std::string& address);

    // Stop server safely
    void stop();

    // Send message to UI
    bool send(const std::string& msg);

    // Set callback for incoming messages from UI
    using MessageHandler = std::function<void(const std::string&)>;
    void setMessageHandler(MessageHandler handler);

private:
    void listenLoop();

private:
    nng_socket m_socket;
    std::thread m_thread;
    std::atomic<bool> m_running{false};

    MessageHandler m_handler = nullptr;
};
