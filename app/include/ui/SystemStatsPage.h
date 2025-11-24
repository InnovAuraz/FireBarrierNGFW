#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QGridLayout>

class SystemStatsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SystemStatsPage(QWidget* parent = nullptr);

public slots:
    void updateStatsPayload(double cpuUsage,
                            double memUsage,
                            double networkSent,
                            double networkReceived);

private:
    QLabel* cpuLabel = nullptr;
    QLabel* memLabel = nullptr;
    QLabel* sentLabel = nullptr;
    QLabel* recvLabel = nullptr;

    QLabel* makeValueLabel(const QString& text);
};
