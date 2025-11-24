#pragma once

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>

class ToastNotification : public QWidget
{
    Q_OBJECT

public:
    explicit ToastNotification(QWidget* parent = nullptr);

public slots:
    void showToast(const QString& message, int durationMs = 3000);

private slots:
    void hideToast();

private:
    QLabel* messageLabel = nullptr;
    QTimer* autoHideTimer = nullptr;
};
