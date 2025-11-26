#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QThread>
#include <atomic>

extern "C" {
#include <nng/nng.h>
#include <nng/protocol/pair0/pair.h>
}

// Forward declarations from your JsonProtocol structures
struct UiResponse;
struct DaemonEvent;

class DaemonClientWorker : public QObject
{
    Q_OBJECT
public:
    explicit DaemonClientWorker(QString endpoint, QObject *parent = nullptr);
    ~DaemonClientWorker();

    void start();
    void stop();
    void requestStopFromOwner();

public slots:
    void sendJson(const QByteArray &json);
    void ioLoop();

signals:
    void connectionEstablished();
    void connectionError(const QString &msg);
    void uiResponseReceived(const UiResponse &resp);
    void daemonEventReceived(const DaemonEvent &evt);

private:
    bool ensureSocket();
    void stopSocket();

private:
    QString m_endpoint;
    std::atomic<bool> m_running {false};

    nng_socket *m_socket = nullptr;
};
