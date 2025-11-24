#pragma once

#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>

class ModelPage : public QWidget
{
    Q_OBJECT

public:
    explicit ModelPage(QWidget* parent = nullptr);

public slots:
    void updateModelInfo(const QVariantMap& data);

private:
    QLabel* versionLabel = nullptr;
    QLabel* roundLabel = nullptr;
    QLabel* compressionLabel = nullptr;
    QLabel* formatLabel = nullptr;
    QLabel* timestampLabel = nullptr;

    QTableWidget* historyTable = nullptr;

    void setupHistoryTable();
};
