#include "MainWindow.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QAction>
#include <QDebug>
#include <QVariantMap>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonDocument>

#include "JsonProtocol.h"  // for UIStatusPayload, UIStatsPayload, FlowSnapshot, DaemonEvent

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("FireBarrier"));
    resize(1100, 700);

    // Core controller & tray
    controller = new UiController(this);
    tray       = new TrayHandler(this);

    // Build UI structure
    buildToolbar();
    buildLayout();

    // Initialize controller (load config, start subscriber, etc.)
    if (!controller->initialize()) {
        // Initialization errors will be emitted via controllerError signal
        qWarning() << "UiController initialization failed";
    }

    // Initialize tray AFTER we have a controller & window
    tray->initialize(this, controller);

    // Connect everything
    connectSignals();

    // Ask for initial data
    controller->refreshStatus();
    controller->refreshStats();
    controller->refreshFlows();
}

MainWindow::~MainWindow()
{
    // All children deleted by QObject parent/child mechanism
}

// ----------------------------------------------------
// UI building
// ----------------------------------------------------

void MainWindow::buildToolbar()
{
    QToolBar* toolbar = addToolBar(QStringLiteral("Monitoring"));
    toolbar->setObjectName(QStringLiteral("mainToolbar"));
    toolbar->setMovable(false);

    QAction* startAction = toolbar->addAction(QStringLiteral("Start Monitoring"));
    QAction* stopAction  = toolbar->addAction(QStringLiteral("Stop Monitoring"));

    connect(startAction, &QAction::triggered,
            this, &MainWindow::onStartMonitoring);

    connect(stopAction, &QAction::triggered,
            this, &MainWindow::onStopMonitoring);
}

void MainWindow::buildLayout()
{
    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    auto* rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // Alert banner at top
    alertBanner = new AlertBanner(central);
    rootLayout->addWidget(alertBanner);

    // Center area: navigation + stacked pages
    auto* centerLayout = new QHBoxLayout();
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(0);

    nav   = new NavigationPanel(central);
    stack = new QStackedWidget(central);

    // Pages
    statusPage = new SystemStatusPage(stack);
    statsPage  = new SystemStatsPage(stack);
    flowsPage  = new FlowsPage(stack);
    alertsPage = new AlertsPage(stack);
    modelPage  = new ModelPage(stack);

    stack->addWidget(statusPage); // index 0 → SystemStatus
    stack->addWidget(statsPage);  // index 1 → SystemStats
    stack->addWidget(flowsPage);  // index 2 → Flows
    stack->addWidget(alertsPage); // index 3 → Alerts
    stack->addWidget(modelPage);  // index 4 → Model

    centerLayout->addWidget(nav);
    centerLayout->addWidget(stack, 1);

    rootLayout->addLayout(centerLayout);

    // Footer
    footer = new StatusFooter(central);
    rootLayout->addWidget(footer);

    // Toast notification (floating, parented to central)
    toast = new ToastNotification(central);
    // Initial positioning (rough bottom-right; not dynamic yet)
    toast->move(width() - 300, height() - 120);
}

// ----------------------------------------------------
// Signal wiring
// ----------------------------------------------------

void MainWindow::connectSignals()
{
    // Navigation panel → switch stacked page
    connect(nav, &NavigationPanel::pageSelected,
            this, &MainWindow::onPageSelected);

    // Controller → status (adapt from UIStatusPayload → slot args)
    connect(controller, &UiController::statusUpdated,
            this, [this](const UIStatusPayload& s) {
                onStatusUpdated(
                    s.daemonAlive,
                    s.pythonOnline,
                    s.captureRunning,
                    s.uptimeSec,
                    s.flowsProcessed,
                    s.modelVersion,
                    s.lastError
                );
            });

    // Controller → stats
    connect(controller, &UiController::statsUpdated,
            this, [this](const UIStatsPayload& s) {
                onStatsUpdated(
                    s.cpuUsage,
                    s.memUsage,
                    s.networkSent,
                    s.networkReceived
                );
            });

    // Controller → flows: convert QVector<FlowSnapshot> → QVector<QVariantMap>
    connect(controller, &UiController::flowsUpdated,
            this, [this](const QVector<FlowSnapshot>& flows) {
                QVector<QVariantMap> list;
                list.reserve(flows.size());

                for (const auto& f : flows) {
                    QVariantMap m;
                    m.insert(QStringLiteral("src_ip"),         f.srcIp);
                    m.insert(QStringLiteral("dst_ip"),         f.dstIp);
                    m.insert(QStringLiteral("src_port"),       f.srcPort);
                    m.insert(QStringLiteral("dst_port"),       f.dstPort);
                    m.insert(QStringLiteral("protocol"),       f.protocol);
                    m.insert(QStringLiteral("first_seen_ms"),  static_cast<qulonglong>(f.firstSeenMs));
                    m.insert(QStringLiteral("last_seen_ms"),   static_cast<qulonglong>(f.lastSeenMs));
                    m.insert(QStringLiteral("bytes_forward"),  static_cast<qulonglong>(f.bytesForward));
                    m.insert(QStringLiteral("bytes_reverse"),  static_cast<qulonglong>(f.bytesReverse));
                    m.insert(QStringLiteral("packets_forward"),static_cast<qulonglong>(f.packetsForward));
                    m.insert(QStringLiteral("packets_reverse"),static_cast<qulonglong>(f.packetsReverse));
                    list.append(m);
                }

                onFlowsUpdated(list);
            });

    // Controller → daemon alerts
    connect(controller, &UiController::daemonAlertReceived,
            this, [this](const DaemonEvent& evt) {
                // Extract payload as QVariantMap for AlertsPage
                QVariantMap map;

                if (evt.payload.isObject()) {
                    map = evt.payload.toObject().toVariantMap();
                } else if (evt.payload.isString()) {
                    map.insert(QStringLiteral("message"), evt.payload.toString());
                }

                // Attach type/timestamp if present in payload or DaemonEvent
                map.insert(QStringLiteral("type"), QStringLiteral("daemon_alert"));

                // Add to Alerts page
                alertsPage->addAlert(map);

                // Build message string for banner/toast
                QString msg;
                if (map.contains(QStringLiteral("message"))) {
                    msg = map.value(QStringLiteral("message")).toString();
                } else {
                    // Fallback: dump minimal JSON
                    QJsonObject obj = evt.payload.toObject();
                    msg = QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
                }

                onDaemonAlert(msg);
            });

    // Controller → daemon errors
    connect(controller, &UiController::daemonErrorReceived,
            this, [this](const DaemonEvent& evt) {
                QVariantMap map;

                if (evt.payload.isObject()) {
                    map = evt.payload.toObject().toVariantMap();
                } else if (evt.payload.isString()) {
                    map.insert(QStringLiteral("message"), evt.payload.toString());
                }

                map.insert(QStringLiteral("type"), QStringLiteral("daemon_error"));

                // Optionally log errors as alerts as well
                alertsPage->addAlert(map);

                QString msg;
                if (map.contains(QStringLiteral("message"))) {
                    msg = map.value(QStringLiteral("message")).toString();
                } else {
                    QJsonObject obj = evt.payload.toObject();
                    msg = QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
                }

                onDaemonError(msg);
            });

    // Controller → model updates
    connect(controller, &UiController::daemonModelUpdateReceived,
            this, [this](const DaemonEvent& evt) {
                QVariantMap data;

                if (evt.payload.isObject()) {
                    data = evt.payload.toObject().toVariantMap();
                }

                onModelUpdate(data);
            });

    // Controller → generic errors
    connect(controller, &UiController::controllerError,
            this, [this](const QString& err) {
                footer->updateLastError(err);
                if (toast)
                    toast->showToast(QStringLiteral("Error: ") + err);
            });
}

// ----------------------------------------------------
// Slots: navigation
// ----------------------------------------------------

void MainWindow::onPageSelected(UiPage page)
{
    switch (page) {
    case UiPage::SystemStatus:
        stack->setCurrentIndex(0);
        break;
    case UiPage::SystemStats:
        stack->setCurrentIndex(1);
        break;
    case UiPage::Flows:
        stack->setCurrentIndex(2);
        break;
    case UiPage::Alerts:
        stack->setCurrentIndex(3);
        break;
    case UiPage::Model:
        stack->setCurrentIndex(4);
        break;
    }
}

// ----------------------------------------------------
// Slots: toolbar actions
// ----------------------------------------------------

void MainWindow::onStartMonitoring()
{
    if (toast)
        toast->showToast(QStringLiteral("Starting daemon..."));
    controller->startDaemon();
}

void MainWindow::onStopMonitoring()
{
    if (toast)
        toast->showToast(QStringLiteral("Stopping daemon..."));
    controller->stopDaemon();
}

// ----------------------------------------------------
// Slots: controller → UI (status/stats/flows/events)
// ----------------------------------------------------

void MainWindow::onStatusUpdated(bool daemonAlive,
                                 bool pythonOnline,
                                 bool captureRunning,
                                 quint64 uptimeSec,
                                 quint64 flowsProcessed,
                                 const QString& modelVersion,
                                 const QString& lastError)
{
    // Update the status page
    if (statusPage) {
        statusPage->updateStatusPayload(
            daemonAlive,
            pythonOnline,
            captureRunning,
            uptimeSec,
            flowsProcessed,
            modelVersion,
            lastError
        );
    }

    // Update footer
    if (footer) {
        footer->updateDaemonAlive(daemonAlive);
        footer->updatePythonOnline(pythonOnline);
        footer->updateCaptureRunning(captureRunning);
        footer->updateModelVersion(modelVersion);
        footer->updateLastError(lastError);
    }

    // Update tray running state
    if (tray) {
        tray->setDaemonRunning(daemonAlive);
    }
}

void MainWindow::onStatsUpdated(double cpu,
                                double mem,
                                double sent,
                                double recv)
{
    if (statsPage) {
        statsPage->updateStatsPayload(cpu, mem, sent, recv);
    }
}

void MainWindow::onFlowsUpdated(const QVector<QVariantMap>& flows)
{
    if (flowsPage) {
        flowsPage->updateFlows(flows);
    }
}

void MainWindow::onDaemonAlert(const QString& msg)
{
    // Requirement: tray + toast + banner
    if (alertBanner)
        alertBanner->showAlert(msg);

    if (toast)
        toast->showToast(QStringLiteral("Alert: ") + msg);

    if (tray)
        tray->showAlertNotification(QStringLiteral("FireBarrier Alert"), msg);

    // Optional: minimize main window on alert (per earlier requirement)
    showMinimized();
}

void MainWindow::onDaemonError(const QString& msg)
{
    if (toast)
        toast->showToast(QStringLiteral("Daemon error: ") + msg);

    if (footer)
        footer->updateLastError(msg);
}

void MainWindow::onModelUpdate(const QVariantMap& data)
{
    if (modelPage) {
        modelPage->updateModelInfo(data);
    }

    if (toast) {
        QString v = data.value(QStringLiteral("model_version"), QStringLiteral("-")).toString();
        toast->showToast(QStringLiteral("Model updated: ") + v);
    }
}
