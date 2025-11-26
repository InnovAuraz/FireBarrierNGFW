#pragma once

#include <QObject>
#include <QThread>
#include <QByteArray>
#include <QString>
#include <atomic>

// Forward declarations for parsed protocol objects
struct UiResponse;
struct DaemonEvent;

// Forward declare worker class
class DaemonClientWorker;

class DaemonClient : public QObject
{
    Q_OBJECT
public:
    explicit DaemonClient(QObject *parent = nullptr);
    ~DaemonClient();

    void setEndpoint(const QString &endpoint);
    QString endpoint() const;

    // NEW — explicit lifecycle control
    bool startClient();      // start worker + thread
    void stopClient();       // stop worker + thread

    // High-level UI actions
    void requestStatus();
    void requestStats();
    void requestFlows();

signals:
    void uiResponseReceived(const UiResponse &resp);
    void daemonEventReceived(const DaemonEvent &evt);
    void connectionEstablished();
    void connectionError(const QString &error);

    // Used internally to send JSON to worker thread
    void sendJson(const QByteArray &json);

private:
    QByteArray buildRequestJson(const QString &action) const;

private:
    QString m_endpoint = "tcp://127.0.0.1:6001";

    // Worker thread + worker object
    QThread m_ioThread;
    DaemonClientWorker *m_worker = nullptr;

    std::atomic<bool> m_running {false};

    // These are now unused but kept so that the .cpp still compiles safely.
    // They will be NOOPs after patching DaemonClient.cpp.
    bool startSocket() { return true; }
    void ioThreadLoop() {}
    void *m_socket = nullptr;
};
