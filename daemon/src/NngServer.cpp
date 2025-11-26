#include "NngServer.h"

#include <iostream> // optional (can be removed if you don't want std::cerr)

// Constructor / Destructor
NngServer::NngServer()
{
    m_socket = NNG_SOCKET_INITIALIZER;
}

NngServer::~NngServer()
{
    stop();
}

// Start server and bind to address (e.g. "tcp://127.0.0.1:6001")
bool NngServer::start(const std::string& address)
{
    if (m_running.load())
        return false; // already running

    int rv = nng_pair0_open(&m_socket);
    if (rv != 0)
    {
        // std::cerr << "nng_pair0_open failed: " << nng_strerror(rv) << "\n";
        return false;
    }

    rv = nng_listen(m_socket, address.c_str(), nullptr, 0);
    if (rv != 0)
    {
        // std::cerr << "nng_listen failed: " << nng_strerror(rv) << "\n";
        nng_close(m_socket);
        m_socket = NNG_SOCKET_INITIALIZER;
        return false;
    }

    m_running.store(true);
    m_thread = std::thread(&NngServer::listenLoop, this);
    return true;
}

// Stop server and join thread
void NngServer::stop()
{
    if (!m_running.load())
        return;

    m_running.store(false);

    // Closing the socket will unblock nng_recv in listenLoop
    if (m_socket.id != 0)
    {
        nng_close(m_socket);
        m_socket = NNG_SOCKET_INITIALIZER;
    }

    if (m_thread.joinable())
        m_thread.join();
}

// Send message to UI
bool NngServer::send(const std::string& msg)
{
    if (!m_running.load() || m_socket.id == 0)
        return false;

    int rv = nng_send(m_socket, (void*)msg.data(), msg.size(), 0);
    if (rv != 0)
    {
        // std::cerr << "nng_send failed: " << nng_strerror(rv) << "\n";
        return false;
    }
    return true;
}

// Set handler for messages coming from UI
void NngServer::setMessageHandler(MessageHandler handler)
{
    m_handler = std::move(handler);
}

// Internal receive loop
void NngServer::listenLoop()
{
    while (m_running.load())
    {
        char* buf = nullptr;
        size_t sz = 0;

        int rv = nng_recv(m_socket, &buf, &sz, NNG_FLAG_ALLOC);
        if (rv != 0)
        {
            // Socket closed or error; on shutdown we just exit
            if (!m_running.load())
                break;

            // std::cerr << "nng_recv failed: " << nng_strerror(rv) << "\n";
            continue;
        }

        std::string msg(buf, sz);
        nng_free(buf, sz);

        if (m_handler)
        {
            // Handler is expected to be lightweight / non-blocking
            m_handler(msg);
        }
    }
}
