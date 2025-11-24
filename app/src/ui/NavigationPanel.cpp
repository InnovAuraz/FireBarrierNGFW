#include "ui/NavigationPanel.h"
#include <QStyle>
#include <QButtonGroup>

NavigationPanel::NavigationPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    // Create buttons
    statusBtn = new QPushButton("System Status");
    statsBtn  = new QPushButton("System Stats");
    flowsBtn  = new QPushButton("Flows");
    alertsBtn = new QPushButton("Alerts");
    modelBtn  = new QPushButton("Model");

    // Convert them into flat sidebar-style buttons
    for (QPushButton* btn : {statusBtn, statsBtn, flowsBtn, alertsBtn, modelBtn}) {
        btn->setCheckable(true);
        btn->setFlat(true);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        btn->setMinimumHeight(40);
        layout->addWidget(btn);

        // objectNames for QSS later
        btn->setObjectName("navButton");
    }

    layout->addStretch();

    // Make "System Status" selected by default
    activeBtn = statusBtn;
    activeBtn->setChecked(true);

    // Connect click handlers
    connect(statusBtn, &QPushButton::clicked, this, &NavigationPanel::onStatusClicked);
    connect(statsBtn,  &QPushButton::clicked, this, &NavigationPanel::onStatsClicked);
    connect(flowsBtn,  &QPushButton::clicked, this, &NavigationPanel::onFlowsClicked);
    connect(alertsBtn, &QPushButton::clicked, this, &NavigationPanel::onAlertsClicked);
    connect(modelBtn,  &QPushButton::clicked, this, &NavigationPanel::onModelClicked);

    setLayout(layout);
}

void NavigationPanel::setActiveButton(QPushButton* btn)
{
    if (activeBtn == btn)
        return;

    if (activeBtn)
        activeBtn->setChecked(false);

    activeBtn = btn;
    activeBtn->setChecked(true);
}

void NavigationPanel::onStatusClicked()
{
    setActiveButton(statusBtn);
    emit pageSelected(UiPage::SystemStatus);
}

void NavigationPanel::onStatsClicked()
{
    setActiveButton(statsBtn);
    emit pageSelected(UiPage::SystemStats);
}

void NavigationPanel::onFlowsClicked()
{
    setActiveButton(flowsBtn);
    emit pageSelected(UiPage::Flows);
}

void NavigationPanel::onAlertsClicked()
{
    setActiveButton(alertsBtn);
    emit pageSelected(UiPage::Alerts);
}

void NavigationPanel::onModelClicked()
{
    setActiveButton(modelBtn);
    emit pageSelected(UiPage::Model);
}
