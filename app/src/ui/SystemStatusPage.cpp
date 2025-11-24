#include "ui/SystemStatusPage.h"
#include <QFont>

SystemStatusPage::SystemStatusPage(QWidget* parent)
    : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);

    // Title label (QSS can style this later)
    auto* title = new QLabel("System Status", this);
    title->setObjectName("pageTitle");
    title->setFont(QFont("Segoe UI", 18, QFont::Bold));
    mainLayout->addWidget(title);

    // Grid layout for values
    auto* grid = new QGridLayout();
    grid->setSpacing(12);

    daemonLabel = makeValueLabel("Daemon: -");
    pythonLabel = makeValueLabel("Python: -");
    captureLabel = makeValueLabel("Capture: -");
    uptimeLabel = makeValueLabel("Uptime: -");
    flowsLabel = makeValueLabel("Flows Processed: -");
    modelLabel = makeValueLabel("Model Version: -");
    errorLabel = makeValueLabel("");

    // Place widgets
    grid->addWidget(daemonLabel,    0, 0);
    grid->addWidget(pythonLabel,    0, 1);
    grid->addWidget(captureLabel,   1, 0);
    grid->addWidget(uptimeLabel,    1, 1);
    grid->addWidget(flowsLabel,     2, 0);
    grid->addWidget(modelLabel,     2, 1);

    mainLayout->addLayout(grid);
    mainLayout->addWidget(errorLabel);

    setLayout(mainLayout);
}

QLabel* SystemStatusPage::makeValueLabel(const QString& text)
{
    auto* lbl = new QLabel(text, this);
    lbl->setObjectName("statusValueLabel");

    lbl->setFont(QFont("Segoe UI", 12));
    return lbl;
}

void SystemStatusPage::updateStatusPayload(bool daemonAlive,
                                           bool pythonOnline,
                                           bool captureRunning,
                                           quint64 uptimeSec,
                                           quint64 flowsProcessed,
                                           const QString& modelVersion,
                                           const QString& lastError)
{
    daemonLabel->setText(QString("Daemon: %1").arg(daemonAlive ? "Online" : "Offline"));
    pythonLabel->setText(QString("Python: %1").arg(pythonOnline ? "Online" : "Offline"));
    captureLabel->setText(QString("Capture: %1").arg(captureRunning ? "Running" : "Stopped"));

    uptimeLabel->setText(QString("Uptime: %1 sec").arg(uptimeSec));
    flowsLabel->setText(QString("Flows Processed: %1").arg(flowsProcessed));
    modelLabel->setText(QString("Model Version: %1").arg(modelVersion));

    if (lastError.isEmpty()) {
        errorLabel->setText("");
    } else {
        errorLabel->setText("Last Error: " + lastError);
        errorLabel->setStyleSheet("color: #F44336; font-weight: bold;");
    }
}
