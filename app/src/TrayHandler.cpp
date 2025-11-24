#include "TrayHandler.h"
#include "UiController.h"

#include <QMainWindow>
#include <QApplication>
#include <QSystemTrayIcon>
#include <QIcon>
#include <QMenu>
#include <QAction>
#include <QDebug>
#include <QTimer>

// ----------------------------------------------------------
// Constructor / Destructor
// ----------------------------------------------------------

TrayHandler::TrayHandler(QObject *parent)
    : QObject(parent)
{
}

TrayHandler::~TrayHandler()
{
    if (m_trayIcon) {
        m_trayIcon->hide();
        delete m_trayIcon;
    }
    delete m_menu;
}

// ----------------------------------------------------------
// Public API
// ----------------------------------------------------------

void TrayHandler::initialize(QMainWindow *mainWindow, UiController *controller)
{
    m_mainWindow = mainWindow;
    m_controller = controller;

    // Load icons (expected to be bundled in resources.qrc)
    m_iconNormal = QIcon(":/icons/firebarrier_normal.png");
    m_iconAlert  = QIcon(":/icons/firebarrier_alert.png");

    createMenu();
    createTrayIcon();

    // Connect tray menu actions to controller signals
    connect(this, &TrayHandler::startMonitoringRequested,
            m_controller, &UiController::startDaemon);

    connect(this, &TrayHandler::stopMonitoringRequested,
            m_controller, &UiController::stopDaemon);
}

void TrayHandler::setDaemonRunning(bool running)
{
    m_daemonRunning = running;

    // Update menu items
    m_startAction->setEnabled(!running);
    m_stopAction->setEnabled(running);

    // Update tooltip
    QString tip = running
            ? QStringLiteral("FireBarrier — Monitoring active")
            : QStringLiteral("FireBarrier — Monitoring stopped");

    if (m_trayIcon)
        m_trayIcon->setToolTip(tip);

    // Restore normal icon
    if (m_trayIcon)
        m_trayIcon->setIcon(m_iconNormal);
}

void TrayHandler::showAlertNotification(const QString &title, const QString &message)
{
    if (!m_trayIcon)
        return;

    // Switch icon temporarily
    m_trayIcon->setIcon(m_iconAlert);

    // Show system notification
    m_trayIcon->showMessage(title, message, QSystemTrayIcon::Critical);

    // Optional: restore normal icon after a delay
    QTimer::singleShot(3000, this, [this]() {
        if (m_trayIcon)
            m_trayIcon->setIcon(m_iconNormal);
    });
}

// ----------------------------------------------------------
// Private methods
// ----------------------------------------------------------

void TrayHandler::createTrayIcon()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qWarning() << "System tray not available on this platform.";
        return;
    }

    if (m_trayIcon)
        return; // already created

    m_trayIcon = new QSystemTrayIcon(m_iconNormal, this);
    m_trayIcon->setToolTip(QStringLiteral("FireBarrier — Monitoring"));

    connect(m_trayIcon, &QSystemTrayIcon::activated,
            this, &TrayHandler::onTrayIconActivated);

    m_trayIcon->setContextMenu(m_menu);
    m_trayIcon->show();
}

void TrayHandler::createMenu()
{
    m_menu = new QMenu();

    m_showAction  = new QAction(QStringLiteral("Show FireBarrier"), m_menu);
    m_startAction = new QAction(QStringLiteral("Start Monitoring"), m_menu);
    m_stopAction  = new QAction(QStringLiteral("Stop Monitoring"), m_menu);
    m_quitAction  = new QAction(QStringLiteral("Quit"), m_menu);

    connect(m_showAction,  &QAction::triggered, this, &TrayHandler::onShowWindow);
    connect(m_startAction, &QAction::triggered, this, &TrayHandler::onStartMonitoring);
    connect(m_stopAction,  &QAction::triggered, this, &TrayHandler::onStopMonitoring);
    connect(m_quitAction,  &QAction::triggered, this, &TrayHandler::onQuit);

    m_menu->addAction(m_showAction);
    m_menu->addSeparator();
    m_menu->addAction(m_startAction);
    m_menu->addAction(m_stopAction);
    m_menu->addSeparator();
    m_menu->addAction(m_quitAction);

    // Defaults
    m_startAction->setEnabled(true);
    m_stopAction->setEnabled(false);
}

// ----------------------------------------------------------
// Slots
// ----------------------------------------------------------

void TrayHandler::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (!m_mainWindow)
        return;

    switch (reason)
    {
        case QSystemTrayIcon::Trigger:
        case QSystemTrayIcon::DoubleClick:
            // Restore window
            m_mainWindow->showNormal();
            m_mainWindow->activateWindow();
            break;

        case QSystemTrayIcon::Context:
            // Right-click — menu shown automatically by Qt
            break;

        default:
            break;
    }
}

void TrayHandler::onShowWindow()
{
    if (m_mainWindow) {
        m_mainWindow->showNormal();
        m_mainWindow->activateWindow();
    }
}

void TrayHandler::onStartMonitoring()
{
    emit startMonitoringRequested();
}

void TrayHandler::onStopMonitoring()
{
    emit stopMonitoringRequested();
}

void TrayHandler::onQuit()
{
    emit quitRequested();
    QApplication::quit();
}
