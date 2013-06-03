#include "mnertclientsetupfifffilesimulatorwidget.h"
#include "ui_mnertclientsetupfifffilesimulator.h"


using namespace MneRtClientPlugin;

MneRtClientSetupFiffFileSimulatorWidget::MneRtClientSetupFiffFileSimulatorWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MneRtClientSetupFiffFileSimulatorWidget)
{
    ui->setupUi(this);
}

MneRtClientSetupFiffFileSimulatorWidget::~MneRtClientSetupFiffFileSimulatorWidget()
{
    delete ui;
}
