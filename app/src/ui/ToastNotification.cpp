#include "ui/ToastNotification.h"
#include <QGraphicsDropShadowEffect>

ToastNotification::ToastNotification(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setWindowFlags(Qt::FramelessWindowHint);

    // Layout
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);

    messageLabel = new QLabel("");
    messageLabel->setObjectName("toastMessage");
    layout->addWidget(messageLabel);

    // Auto-hide timer
    autoHideTimer = new QTimer(this);
    autoHideTimer->setSingleShot(true);
    connect(autoHideTimer, &QTimer::timeout, this, &ToastNotification::hideToast);

    // Base styling (QSS will override later)
    setStyleSheet(
        "background-color: rgba(40, 40, 40, 200);"
        "color: white;"
        "border-radius: 8px;"
        "padding: 4px;"
        "font-weight: bold;"
    );

    // Soft drop shadow for floating look
    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(12);
    shadow->setOffset(0, 3);
    setGraphicsEffect(shadow);

    hide(); // start hidden
}

void ToastNotification::showToast(const QString& message, int durationMs)
{
    messageLabel->setText(message);

    // Show it immediately
    show();
    raise();

    // Restart timer
    autoHideTimer->stop();
    autoHideTimer->start(durationMs);
}

void ToastNotification::hideToast()
{
    hide();
}
