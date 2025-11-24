#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>

class AlertsPage : public QWidget
{
    Q_OBJECT

public:
    explicit AlertsPage(QWidget* parent = nullptr);

public slots:
    void addAlert(const QVariantMap& alertData);

private:
    QTableWidget* table = nullptr;

    void setupTable();
};
