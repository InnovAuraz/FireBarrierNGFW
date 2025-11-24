#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QFuture>

class DaemonClient : public QObject
{
    Q_OBJECT
public:
    explicit DaemonClient(QObject *parent = nullptr);

    // Set REQ endpoint (default: tcp://127.0.0.1:6001)
    void setEndpoint(const QString &endpoint);
    QString endpoint() const;

    // High-level UI requests (async)
    void requestStatus();
    void requestStats();
    void requestFlows();

signals:
    // Emitted when a request successfully completes
    void requestCompleted(const QString &action, const QByteArray &responseJson);

    // Emitted when request fails (connection error, timeout, invalid JSON, etc.)
    void requestFailed(const QString &action, const QString &errorMessage);

private:
    // Internal helper to run an action asynchronously
    void sendRequestAsync(const QString &action);

    // Converts action string into JSON message to send
    QByteArray buildRequestJson(const QString &action) const;

    QString m_endpoint;  // example: tcp://127.0.0.1:6001

    QList<QFuture<void>> m_futures;
};
