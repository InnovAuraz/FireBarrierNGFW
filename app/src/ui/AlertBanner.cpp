#include "ui/AlertBanner.h"
#include <QColor>

AlertBanner::AlertBanner(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 6, 12, 6);
    layout->setSpacing(10);

    messageLabel = new QLabel("");
    messageLabel->setObjectName("alertMessage");

    layout->addWidget(messageLabel);
    setLayout(layout);

    // Default: hidden
    setVisible(false);

    // Auto-hide after 5 seconds
    autoHideTimer = new QTimer(this);
    autoHideTimer->setInterval(5000);
    autoHideTimer->setSingleShot(true);
    connect(autoHideTimer, &QTimer::timeout, this, &AlertBanner::hideBanner);

    // Basic styling (QSS will improve later)
    setStyleSheet(
        "background-color: #FF5252; "
        "color: white; "
        "font-weight: bold; "
        "border-radius: 6px;"
    );
}

void AlertBanner::showAlert(const QString& message)
{
    messageLabel->setText("⚠  " + message);

    // Show immediately
    setVisible(true);

    // Restart timer
    autoHideTimer->stop();
    autoHideTimer->start();
}

void AlertBanner::hideBanner()
{
    setVisible(false);
}
