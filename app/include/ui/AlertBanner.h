#pragma once

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QHBoxLayout>

class AlertBanner : public QWidget
{
    Q_OBJECT

public:
    explicit AlertBanner(QWidget* parent = nullptr);

public slots:
    void showAlert(const QString& message);

private slots:
    void hideBanner();

private:
    QLabel* messageLabel = nullptr;
    QTimer* autoHideTimer = nullptr;
};
