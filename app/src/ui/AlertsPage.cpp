#include "ui/AlertsPage.h"
#include <QHeaderView>
#include <QFont>
#include <QDateTime>
#include <QLabel>

AlertsPage::AlertsPage(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);

    // Title
    auto* title = new QLabel("Alerts", this);
    title->setObjectName("pageTitle");
    title->setFont(QFont("Segoe UI", 18, QFont::Bold));
    layout->addWidget(title);

    // Table setup
    table = new QTableWidget(this);
    table->setObjectName("alertsTable");

    setupTable();

    layout->addWidget(table);
    setLayout(layout);
}

void AlertsPage::setupTable()
{
    table->setColumnCount(3);

    QStringList headers = {
        "Timestamp",
        "Type",
        "Message"
    };

    table->setHorizontalHeaderLabels(headers);

    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
}

void AlertsPage::addAlert(const QVariantMap& alertData)
{
    int row = table->rowCount();
    table->insertRow(row);

    auto set = [&](int col, const QVariant& val) {
        QTableWidgetItem* item = new QTableWidgetItem(val.toString());
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
        table->setItem(row, col, item);
    };

    // Extract timestamp (if exists)
    QString ts;
    if (alertData.contains("timestamp")) {
        qint64 t = alertData.value("timestamp").toLongLong();
        ts = QDateTime::fromMSecsSinceEpoch(t).toString("yyyy-MM-dd hh:mm:ss");
    } else {
        ts = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    }

    // Extract type/message (fallback if missing)
    QString type = alertData.value("type", "alert").toString();
    QString msg  = alertData.value("message", "No details").toString();

    set(0, ts);
    set(1, type);
    set(2, msg);

    // Auto-scroll to bottom
    table->scrollToBottom();
}
