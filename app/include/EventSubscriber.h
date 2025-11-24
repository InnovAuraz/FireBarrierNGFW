#pragma once

#include <QObject>
#include <QThread>
#include <QByteArray>
#include <QString>

class EventSubscriber : public QObject
{
    Q_OBJECT
public:
    explicit EventSubscriber(QObject *parent = nullptr);
    ~EventSubscriber();

    void setEndpoint(const QString &endpoint);
    QString endpoint() const;

    bool start();
    void stop();

signals:
    void eventReceived(const QByteArray &eventJson);
    void subscriberError(const QString &errorMessage);

private:
    void workerLoop();
    void cleanupSocket();

private:
    QString m_endpoint;
    QThread m_thread;

    bool m_running;

    // Store socket as a raw integer handle
    bool m_socketInitialized;
};
