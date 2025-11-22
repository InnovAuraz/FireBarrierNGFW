#pragma once
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include "nng/nng.h"
#include <nng/protocol/pair0/pair.h>

class NngClient
{
public:
    NngClient();
    ~NngClient();

    // Connect to Python server at given address
    bool start(const std::string& address);

    // Disconnect safely and stop thread
    void stop();

    // Send JSON message to Python
    bool send(const std::string& msg);

    // Handler for messages FROM Python
    using MessageHandler = std::function<void(const std::string&)>;
    void setMessageHandler(MessageHandler handler);

    // Check if the socket is connected
    bool isConnected() const;

private:
    void receiveLoop();
    bool reconnect();

private:
    nng_socket m_socket;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_connected{false};

    std::string m_address;
    MessageHandler m_handler = nullptr;
};
