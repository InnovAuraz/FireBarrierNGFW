#include "ui/FlowsPage.h"
#include <QHeaderView>
#include <QLabel>

FlowsPage::FlowsPage(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);

    // Title
    auto* title = new QLabel("Active Network Flows", this);
    title->setObjectName("pageTitle");
    title->setFont(QFont("Segoe UI", 18, QFont::Bold));
    layout->addWidget(title);

    // Table
    table = new QTableWidget(this);
    table->setObjectName("flowsTable");

    setupTable();

    layout->addWidget(table);
    setLayout(layout);
}

void FlowsPage::setupTable()
{
    table->setColumnCount(10);

    QStringList headers = {
        "Src IP", "Dst IP",
        "Src Port", "Dst Port",
        "Protocol",
        "First Seen (ms)", "Last Seen (ms)",
        "Bytes Fwd", "Bytes Rev",
        "Packets Fwd/Rev"
    };

    table->setHorizontalHeaderLabels(headers);

    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
}

void FlowsPage::updateFlows(const QVector<QVariantMap>& flows)
{
    table->setRowCount(flows.size());

    for (int row = 0; row < flows.size(); ++row)
    {
        const QVariantMap& f = flows[row];

        auto set = [&](int col, const QVariant& val) {
            QTableWidgetItem* item = new QTableWidgetItem(val.toString());
            item->setFlags(item->flags() ^ Qt::ItemIsEditable);
            table->setItem(row, col, item);
        };

        set(0, f.value("src_ip"));
        set(1, f.value("dst_ip"));
        set(2, f.value("src_port"));
        set(3, f.value("dst_port"));
        set(4, f.value("protocol"));
        set(5, f.value("first_seen_ms"));
        set(6, f.value("last_seen_ms"));
        set(7, f.value("bytes_forward"));
        set(8, f.value("bytes_reverse"));

        QString packets = QString("%1 / %2")
                .arg(f.value("packets_forward").toInt())
                .arg(f.value("packets_reverse").toInt());

        set(9, packets);
    }
}
