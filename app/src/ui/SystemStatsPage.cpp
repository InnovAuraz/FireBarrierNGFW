#include "ui/SystemStatsPage.h"
#include <QFont>

SystemStatsPage::SystemStatsPage(QWidget* parent)
    : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);

    // Title label
    auto* title = new QLabel("System Statistics", this);
    title->setObjectName("pageTitle");
    title->setFont(QFont("Segoe UI", 18, QFont::Bold));
    mainLayout->addWidget(title);

    // Grid for stats fields
    auto* grid = new QGridLayout();
    grid->setSpacing(12);

    cpuLabel  = makeValueLabel("CPU Usage: -");
    memLabel  = makeValueLabel("Memory Usage: -");
    sentLabel = makeValueLabel("Network Sent: -");
    recvLabel = makeValueLabel("Network Received: -");

    grid->addWidget(cpuLabel,  0, 0);
    grid->addWidget(memLabel,  0, 1);
    grid->addWidget(sentLabel, 1, 0);
    grid->addWidget(recvLabel, 1, 1);

    mainLayout->addLayout(grid);

    setLayout(mainLayout);
}

QLabel* SystemStatsPage::makeValueLabel(const QString& text)
{
    auto* lbl = new QLabel(text, this);
    lbl->setObjectName("statsValueLabel");

    lbl->setFont(QFont("Segoe UI", 12));
    return lbl;
}

void SystemStatsPage::updateStatsPayload(double cpuUsage,
                                         double memUsage,
                                         double networkSent,
                                         double networkReceived)
{
    cpuLabel->setText(QString("CPU Usage: %1%").arg(cpuUsage, 0, 'f', 2));
    memLabel->setText(QString("Memory Usage: %1%").arg(memUsage, 0, 'f', 2));

    sentLabel->setText(QString("Network Sent: %1 KB/s")
                       .arg(networkSent, 0, 'f', 2));

    recvLabel->setText(QString("Network Received: %1 KB/s")
                       .arg(networkReceived, 0, 'f', 2));
}
