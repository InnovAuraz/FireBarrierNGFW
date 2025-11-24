#include "ui/ModelPage.h"
#include <QHeaderView>
#include <QFont>
#include <QDateTime>

ModelPage::ModelPage(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);

    // Title
    auto* title = new QLabel("ML Model Information", this);
    title->setObjectName("pageTitle");
    title->setFont(QFont("Segoe UI", 18, QFont::Bold));
    layout->addWidget(title);

    // Info labels
    versionLabel     = new QLabel("Model Version: -", this);
    roundLabel       = new QLabel("Federated Round: -", this);
    compressionLabel = new QLabel("Compression: -", this);
    formatLabel      = new QLabel("Format: -", this);
    timestampLabel   = new QLabel("Timestamp: -", this);

    versionLabel->setObjectName("modelInfoLabel");
    roundLabel->setObjectName("modelInfoLabel");
    compressionLabel->setObjectName("modelInfoLabel");
    formatLabel->setObjectName("modelInfoLabel");
    timestampLabel->setObjectName("modelInfoLabel");

    layout->addWidget(versionLabel);
    layout->addWidget(roundLabel);
    layout->addWidget(compressionLabel);
    layout->addWidget(formatLabel);
    layout->addWidget(timestampLabel);

    // History table
    historyTable = new QTableWidget(this);
    historyTable->setObjectName("modelHistoryTable");
    setupHistoryTable();
    layout->addWidget(historyTable);

    setLayout(layout);
}

void ModelPage::setupHistoryTable()
{
    historyTable->setColumnCount(5);

    QStringList headers = {
        "Timestamp",
        "Model Version",
        "Round",
        "Compression",
        "Format"
    };

    historyTable->setHorizontalHeaderLabels(headers);

    historyTable->horizontalHeader()->setStretchLastSection(true);
    historyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    historyTable->verticalHeader()->setVisible(false);
    historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    historyTable->setSelectionMode(QAbstractItemView::SingleSelection);
}

void ModelPage::updateModelInfo(const QVariantMap& data)
{
    // Extract values (fallbacks included)
    QString version     = data.value("model_version", "-").toString();
    int round           = data.value("federated_round", -1).toInt();
    QString comp        = data.value("compression", "-").toString();
    QString fmt         = data.value("format", "-").toString();
    qint64 timestampVal = data.value("timestamp", 0).toLongLong();

    QString timestamp;
    if (timestampVal > 0) {
        timestamp = QDateTime::fromMSecsSinceEpoch(timestampVal * 1000)
                        .toString("yyyy-MM-dd hh:mm:ss");
    } else {
        timestamp = "-";
    }

    // Update labels
    versionLabel->setText("Model Version: " + version);
    roundLabel->setText("Federated Round: " + QString::number(round));
    compressionLabel->setText("Compression: " + comp);
    formatLabel->setText("Format: " + fmt);
    timestampLabel->setText("Timestamp: " + timestamp);

    // Add to history table
    int row = historyTable->rowCount();
    historyTable->insertRow(row);

    auto set = [&](int col, const QString& val) {
        QTableWidgetItem* item = new QTableWidgetItem(val);
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
        historyTable->setItem(row, col, item);
    };

    set(0, timestamp);
    set(1, version);
    set(2, QString::number(round));
    set(3, comp);
    set(4, fmt);

    historyTable->scrollToBottom();
}
