#include "ui/StatusFooter.h"
#include <QPalette>

StatusFooter::StatusFooter(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(12);

    daemonLabel = new QLabel("Daemon: ?");
    pythonLabel = new QLabel("Python: ?");
    captureLabel = new QLabel("Capture: ?");
    modelLabel   = new QLabel("Model: -");
    errorLabel   = new QLabel("");

    // Set object names for QSS later
    daemonLabel->setObjectName("statusLabel");
    pythonLabel->setObjectName("statusLabel");
    captureLabel->setObjectName("statusLabel");
    modelLabel->setObjectName("statusLabel");
    errorLabel->setObjectName("statusErrorLabel");

    layout->addWidget(daemonLabel);
    layout->addWidget(pythonLabel);
    layout->addWidget(captureLabel);
    layout->addWidget(modelLabel);
    layout->addStretch();
    layout->addWidget(errorLabel);

    setLayout(layout);
}

void StatusFooter::updateLabel(QLabel* lbl, bool ok, const QString& name)
{
    QString color = ok ? "#4CAF50" : "#F44336"; // green / red
    QString text  = name + ": " + (ok ? "Yes" : "No");

    lbl->setText(text);
    lbl->setStyleSheet("color: " + color + "; font-weight: bold;");
}

void StatusFooter::updateDaemonAlive(bool alive)
{
    updateLabel(daemonLabel, alive, "Daemon");
}

void StatusFooter::updatePythonOnline(bool online)
{
    updateLabel(pythonLabel, online, "Python");
}

void StatusFooter::updateCaptureRunning(bool running)
{
    updateLabel(captureLabel, running, "Capture");
}

void StatusFooter::updateModelVersion(const QString& version)
{
    modelLabel->setText("Model: " + version);
    modelLabel->setStyleSheet("color: #2196F3; font-weight: bold;");
}

void StatusFooter::updateLastError(const QString& error)
{
    if (error.isEmpty()) {
        errorLabel->setText("");
        return;
    }

    errorLabel->setText("Error: " + error);
    errorLabel->setStyleSheet("color: #FF9800; font-weight: bold;");
}
