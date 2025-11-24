#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QIcon>

class QMainWindow;
class UiController;

class TrayHandler : public QObject
{
    Q_OBJECT

public:
    explicit TrayHandler(QObject *parent = nullptr);
    ~TrayHandler();

    // Must be called after MainWindow is constructed
    void initialize(QMainWindow *mainWindow, UiController *controller);

    // Called when daemon state changes
    void setDaemonRunning(bool running);

    // Called on daemon_alert
    void showAlertNotification(const QString &title, const QString &message);

signals:
    // Emitted when tray menu triggers actions
    void startMonitoringRequested();
    void stopMonitoringRequested();
    void quitRequested();

private slots:
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);

    void onShowWindow();
    void onStartMonitoring();
    void onStopMonitoring();
    void onQuit();

private:
    void createTrayIcon();
    void createMenu();

private:
    QSystemTrayIcon *m_trayIcon = nullptr;
    QMenu           *m_menu     = nullptr;

    QAction *m_showAction  = nullptr;
    QAction *m_startAction = nullptr;
    QAction *m_stopAction  = nullptr;
    QAction *m_quitAction  = nullptr;

    QMainWindow  *m_mainWindow  = nullptr;
    UiController *m_controller  = nullptr;

    bool m_daemonRunning = false;

    // Icons:
    QIcon m_iconNormal;
    QIcon m_iconAlert;
};
