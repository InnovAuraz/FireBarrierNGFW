#pragma once

#include <QMainWindow>
#include <QToolBar>
#include <QStackedWidget>

#include "UiController.h"
#include "TrayHandler.h"

#include "ui/NavigationPanel.h"
#include "ui/SystemStatusPage.h"
#include "ui/SystemStatsPage.h"
#include "ui/FlowsPage.h"
#include "ui/AlertsPage.h"
#include "ui/ModelPage.h"
#include "ui/StatusFooter.h"
#include "ui/AlertBanner.h"
#include "ui/ToastNotification.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    // Navigation
    void onPageSelected(UiPage page);

    // Toolbar actions
    void onStartMonitoring();
    void onStopMonitoring();

    // Controller → UI
    void onStatusUpdated(bool daemonAlive,
                         bool pythonOnline,
                         bool captureRunning,
                         quint64 uptimeSec,
                         quint64 flowsProcessed,
                         const QString& modelVersion,
                         const QString& lastError);

    void onStatsUpdated(double cpu,
                        double mem,
                        double sent,
                        double recv);

    void onFlowsUpdated(const QVector<QVariantMap>& flows);

    void onDaemonAlert(const QString& msg);
    void onDaemonError(const QString& msg);
    void onModelUpdate(const QVariantMap& data);

private:
    // Core controller
    UiController* controller = nullptr;

    // Tray
    TrayHandler* tray = nullptr;

    // UI modules
    NavigationPanel* nav = nullptr;
    QStackedWidget* stack = nullptr;

    SystemStatusPage* statusPage = nullptr;
    SystemStatsPage* statsPage  = nullptr;
    FlowsPage* flowsPage        = nullptr;
    AlertsPage* alertsPage      = nullptr;
    ModelPage* modelPage        = nullptr;

    StatusFooter* footer = nullptr;
    AlertBanner* alertBanner = nullptr;
    ToastNotification* toast = nullptr;

    // UI building helpers
    void buildToolbar();
    void buildLayout();
    void connectSignals();
};
