#include "EventSubscriber.h"

#include <QDebug>
#include <QMetaObject>

extern "C" {
#include <nng/nng.h>
#include <nng/protocol/pubsub0/sub.h>
}

EventSubscriber::EventSubscriber(QObject *parent)
    : QObject(parent),
      m_endpoint("tcp://127.0.0.1:6002"),
      m_running(false),
      m_socketInitialized(false)
{
}

EventSubscriber::~EventSubscriber()
{
    stop();
}

void EventSubscriber::setEndpoint(const QString &endpoint)
{
    m_endpoint = endpoint;
}

QString EventSubscriber::endpoint() const
{
    return m_endpoint;
}

bool EventSubscriber::start()
{
    if (m_running)
        return true;

    m_running = true;

    connect(&m_thread, &QThread::started,
            this, &EventSubscriber::workerLoop);

    m_thread.start();
    return true;
}

void EventSubscriber::stop()
{
    if (!m_running)
        return;

    m_running = false;

    m_thread.quit();
    m_thread.wait();
}

void EventSubscriber::cleanupSocket()
{
    // No-op now because socket is local to worker loop
}

// Worker thread entry point
void EventSubscriber::workerLoop()
{
    nng_socket sock;
    int rv = nng_sub0_open(&sock);

    if (rv != 0) {
        emit subscriberError(
            QString("Failed to create SUB socket: %1").arg(nng_strerror(rv)));
        return;
    }

    // Subscribe to all topics
    rv = nng_setopt(sock, NNG_OPT_SUB_SUBSCRIBE, "", 0);
    if (rv != 0) {
        emit subscriberError(
            QString("Failed to subscribe: %1").arg(nng_strerror(rv)));
        nng_close(sock);
        return;
    }

    // Connect
    rv = nng_dial(sock, m_endpoint.toUtf8().constData(), nullptr, 0);
    if (rv != 0) {
        emit subscriberError(
            QString("Failed to connect to daemon at %1: %2")
                .arg(m_endpoint, nng_strerror(rv)));
        nng_close(sock);
        return;
    }

    // Receive loop
    while (m_running) {

        char *msg = nullptr;
        size_t sz = 0;

        rv = nng_recv(sock, &msg, &sz, NNG_FLAG_ALLOC);

        if (rv != 0) {
            if (!m_running)
                break;

            emit subscriberError(
                QString("Event receive error: %1").arg(nng_strerror(rv)));

            QThread::msleep(200);
            continue;
        }

        QByteArray event(msg, sz);
        nng_free(msg, sz);

        emit eventReceived(event);
    }

    // Final cleanup
    nng_close(sock);
}
