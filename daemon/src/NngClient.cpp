#include "NngClient.h"

#include <chrono>
#include <thread>

// -----------------------------------------------------------
// Constructor / Destructor
// -----------------------------------------------------------
NngClient::NngClient()
{
    m_socket = NNG_SOCKET_INITIALIZER;
}

NngClient::~NngClient()
{
    stop();
}

// -----------------------------------------------------------
// Start the client and connect to Python server
// -----------------------------------------------------------
bool NngClient::start(const std::string& address)
{
    if (m_running.load())
        return false; // already running

    m_address = address;

    int rv = nng_pair0_open(&m_socket);
    if (rv != 0)
    {
        m_connected.store(false);
        return false;
    }

    // Attempt initial connection
    rv = nng_dial(m_socket, address.c_str(), nullptr, 0);
    if (rv == 0)
        m_connected.store(true);
    else
        m_connected.store(false);

    m_running.store(true);
    m_thread = std::thread(&NngClient::receiveLoop, this);

    return true;
}

// -----------------------------------------------------------
// Stop the client cleanly
// -----------------------------------------------------------
void NngClient::stop()
{
    if (!m_running.load())
        return;

    m_running.store(false);

    if (m_socket.id != 0)
    {
        nng_close(m_socket);
        m_socket = NNG_SOCKET_INITIALIZER;
    }

    if (m_thread.joinable())
        m_thread.join();

    m_connected.store(false);
}

// -----------------------------------------------------------
// Send a message to Python
// -----------------------------------------------------------
bool NngClient::send(const std::string& msg)
{
    if (!m_running.load() || !m_connected.load())
        return false;

    int rv = nng_send(m_socket, (void*)msg.data(), msg.size(), 0);
    if (rv != 0)
    {
        // Python server disconnected? mark as offline
        m_connected.store(false);
        return false;
    }

    return true;
}

// -----------------------------------------------------------
// Set message handler for Python → Daemon messages
// -----------------------------------------------------------
void NngClient::setMessageHandler(MessageHandler handler)
{
    m_handler = std::move(handler);
}

// -----------------------------------------------------------
// Connection status check
// -----------------------------------------------------------
bool NngClient::isConnected() const
{
    return m_connected.load();
}

// -----------------------------------------------------------
// Background receive loop (auto reconnect)
// -----------------------------------------------------------
void NngClient::receiveLoop()
{
    while (m_running.load())
    {
        if (!m_connected.load())
        {
            reconnect();
            // If still not connected, wait a bit
            if (!m_connected.load())
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }
        }

        char* buf = nullptr;
        size_t sz = 0;

        int rv = nng_recv(m_socket, &buf, &sz, NNG_FLAG_ALLOC);
        if (rv != 0)
        {
            // Disconnected → attempt reconnect
            m_connected.store(false);
            continue;
        }

        std::string msg(buf, sz);
        nng_free(buf, sz);

        if (m_handler)
            m_handler(msg);
    }
}

// -----------------------------------------------------------
// Reconnect to Python server
// -----------------------------------------------------------
bool NngClient::reconnect()
{
    if (m_socket.id == 0)
    {
        // Re-open the socket
        if (nng_pair0_open(&m_socket) != 0)
        {
            m_connected.store(false);
            return false;
        }
    }

    // Try reconnect dial
    int rv = nng_dial(m_socket, m_address.c_str(), nullptr, 0);

    if (rv == 0)
    {
        m_connected.store(true);
        return true;
    }

    m_connected.store(false);
    return false;
}
