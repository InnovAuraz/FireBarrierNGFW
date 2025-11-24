#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>

class FlowsPage : public QWidget
{
    Q_OBJECT

public:
    explicit FlowsPage(QWidget* parent = nullptr);

public slots:
    void updateFlows(const QVector<QVariantMap>& flows); 
    // OR adjust once we define how UiController passes FlowRecord

private:
    QTableWidget* table = nullptr;

    void setupTable();
};
