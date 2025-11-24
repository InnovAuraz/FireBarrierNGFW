#include "DaemonClient.h"

#include <QtConcurrent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>

#include <nng/nng.h>
#include <nng/protocol/reqrep0/req.h>

DaemonClient::DaemonClient(QObject *parent)
    : QObject(parent),
      m_endpoint("tcp://127.0.0.1:6001")
{
}

void DaemonClient::setEndpoint(const QString &endpoint)
{
    m_endpoint = endpoint;
}

QString DaemonClient::endpoint() const
{
    return m_endpoint;
}

// Public request functions
void DaemonClient::requestStatus()
{
    sendRequestAsync("ui_get_status");
}

void DaemonClient::requestStats()
{
    sendRequestAsync("ui_get_stats");
}

void DaemonClient::requestFlows()
{
    sendRequestAsync("ui_get_flows");
}

// Build JSON request
QByteArray DaemonClient::buildRequestJson(const QString &action) const
{
    QJsonObject obj;
    obj["action"] = action;
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

// Async runner using QtConcurrent
void DaemonClient::sendRequestAsync(const QString &action)
{
    const QString endpointCopy = m_endpoint;
    const QByteArray requestJson = buildRequestJson(action);

    // Run the blocking NNG REQ call in a background thread
    m_futures.append(QtConcurrent::run([this, action, endpointCopy, requestJson]() {

        nng_socket sock;
        int rv = nng_req0_open(&sock);
        if (rv != 0) {
            emit requestFailed(action, QString("Failed to create REQ socket: %1").arg(nng_strerror(rv)));
            return;
        }

        // Dial the endpoint
        rv = nng_dial(sock, endpointCopy.toUtf8().constData(), nullptr, 0);
        if (rv != 0) {
            nng_close(sock);
            emit requestFailed(action, QString("Failed to connect to daemon at %1: %2")
                                      .arg(endpointCopy, nng_strerror(rv)));
            return;
        }

        // Send request
        rv = nng_send(sock, (void *)requestJson.data(), requestJson.size(), 0);
        if (rv != 0) {
            nng_close(sock);
            emit requestFailed(action, QString("Failed to send request: %1").arg(nng_strerror(rv)));
            return;
        }

        // Receive reply
        char *reply = nullptr;
        size_t replySize = 0;

        rv = nng_recv(sock, &reply, &replySize, NNG_FLAG_ALLOC);
        if (rv != 0) {
            nng_close(sock);
            emit requestFailed(action, QString("Failed to receive reply: %1").arg(nng_strerror(rv)));
            return;
        }

        QByteArray responseJson(reply, replySize);
        nng_free(reply, replySize);
        nng_close(sock);

        emit requestCompleted(action, responseJson);
    }));
}
