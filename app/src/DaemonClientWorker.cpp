#include "DaemonClientWorker.h"
#include "JsonProtocol.h"

#include "UiDebugLog.h"

#include <QThread>

// ---------------------------
// Constructor / Destructor
// ---------------------------

DaemonClientWorker::DaemonClientWorker(QString endpoint, QObject *parent)
    : QObject(parent),
      m_endpoint(std::move(endpoint))
{
}

DaemonClientWorker::~DaemonClientWorker()
{
    stopSocket();
}

// ---------------------------
// Control
// ---------------------------

void DaemonClientWorker::start()
{
    m_running = true;
}

void DaemonClientWorker::stop()
{
    ui_debug_log("Worker: stop() called");
    m_running = false;

    // Force nng_recv() to abort immediately
    if (m_socket) {
        ui_debug_log("Worker: closing socket to interrupt nng_recv()");
        nng_close(*m_socket);  // <--- THIS breaks recv instantly
    }
}

void DaemonClientWorker::requestStopFromOwner()
{
    ui_debug_log("Worker: requestStopFromOwner called");
    m_running = false;
    stopSocket();  // this will force nng_recv to return NNG_ECLOSED
}

// ---------------------------
// Sending JSON
// ---------------------------

void DaemonClientWorker::sendJson(const QByteArray &json)
{
    if (!ensureSocket())
        return;

    nng_socket sock = *m_socket;

    int rv = nng_send(sock,
                      (void *)json.constData(),
                      json.size(),
                      0);

    if (rv != 0)
    {
        emit connectionError(QStringLiteral("Send error: %1")
                             .arg(nng_strerror(rv)));
    }
}

// ---------------------------
// IO Loop (receiving messages)
// ---------------------------

void DaemonClientWorker::ioLoop()
{
    ui_debug_log("Worker: ioLoop started");

    // Prevent hammering dial immediately after UI starts
    static bool firstLoop = true;
    if (firstLoop) {
        ui_debug_log("Worker: firstLoop delay 500ms");

        QThread::msleep(500);    // 0.5 sec startup delay
        firstLoop = false;
    }

    while (true)
    {
        if (!m_running)
        {
            ui_debug_log("Worker: stop requested, breaking BEFORE recv");
            break;
        }

        if (!ensureSocket())
        {
            QThread::sleep(2);
            continue;
        }

        nng_socket sock = *m_socket;

        char *buf = nullptr;
        size_t sz = 0;

        ui_debug_log("Worker: waiting for nng_recv()");

        int rv = nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC);

        if (!m_running)
        {
            ui_debug_log("Worker: stop requested AFTER recv");
            if (rv == 0 && buf)
                nng_free(buf, sz);
            break;
        }

        if (rv == NNG_ETIMEDOUT)
            continue;

        // 🔥 CRITICAL FIX: break immediately when socket is closed
        if (rv == NNG_ECLOSED)
        {
            ui_debug_log("Worker: socket closed (NNG_ECLOSED), exiting recv loop");
            break;
        }

        if (rv != 0)
        {
            ui_debug_log(QString("Worker: recv error %1 (%2)")
                        .arg(rv)
                        .arg(nng_strerror(rv)));

            emit connectionError(QStringLiteral("Receive error: %1")
                                .arg(nng_strerror(rv)));

            stopSocket();
            QThread::sleep(1);
            continue;
        }

        QByteArray data(buf, int(sz));
        nng_free(buf, sz);

        QString err;

        UiResponse resp;
        if (JsonProtocol::parseUiResponse(data, resp, &err))
        {
            emit uiResponseReceived(resp);
            continue;
        }

        DaemonEvent evt;
        if (JsonProtocol::parseDaemonEvent(data, evt, &err))
        {
            emit daemonEventReceived(evt);
            continue;
        }

        emit connectionError("Unrecognized JSON received");
    }

    ui_debug_log("Worker: ioLoop finished");

}

// ---------------------------
// Socket handling
// ---------------------------

bool DaemonClientWorker::ensureSocket()
{
    if (!m_running)
        return false;

    if (m_socket)
        return true;

    auto *sock = new nng_socket;

    int rv = nng_pair0_open(sock);
    if (rv != 0) {
        emit connectionError(QStringLiteral("PAIR open failed: %1")
                             .arg(nng_strerror(rv)));
        delete sock;
        return false;
    }

    nng_setopt_ms(*sock, NNG_OPT_RECVTIMEO, 2000);
    nng_setopt_ms(*sock, NNG_OPT_SENDTIMEO, 2000);

    rv = nng_dial(*sock, m_endpoint.toUtf8().constData(), nullptr, 0);
    if (rv != 0) {
        emit connectionError(QStringLiteral("Dial failed (%1): %2")
                             .arg(m_endpoint, nng_strerror(rv)));
        nng_close(*sock);
        delete sock;
        return false;
    }

    m_socket = sock;
    emit connectionEstablished();

    ui_debug_log("Worker: nng_dial success, socket connected");

    return true;
}

void DaemonClientWorker::stopSocket()
{
    if (m_socket) {
        // socket already closed in stop(), so no nng_close here
        delete m_socket;
        m_socket = nullptr;
    }
}
