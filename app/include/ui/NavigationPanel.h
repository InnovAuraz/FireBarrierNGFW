#pragma once

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>

enum class UiPage {
    SystemStatus,
    SystemStats,
    Flows,
    Alerts,
    Model
};

class NavigationPanel : public QWidget
{
    Q_OBJECT

public:
    explicit NavigationPanel(QWidget* parent = nullptr);

signals:
    void pageSelected(UiPage page);

private slots:
    void onStatusClicked();
    void onStatsClicked();
    void onFlowsClicked();
    void onAlertsClicked();
    void onModelClicked();

private:
    QPushButton* statusBtn = nullptr;
    QPushButton* statsBtn  = nullptr;
    QPushButton* flowsBtn  = nullptr;
    QPushButton* alertsBtn = nullptr;
    QPushButton* modelBtn  = nullptr;

    void setActiveButton(QPushButton* btn);
    QPushButton* activeBtn = nullptr;
};
