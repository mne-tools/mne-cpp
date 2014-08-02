#include "mnertclientsetupneuromagwidget.h"
#include "ui_mnertclientsetupneuromag.h"


using namespace MneRtClientPlugin;

MneRtClientSetupNeuromagWidget::MneRtClientSetupNeuromagWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MneRtClientSetupNeuromagWidget)
{
    ui->setupUi(this);
}

MneRtClientSetupNeuromagWidget::~MneRtClientSetupNeuromagWidget()
{
    delete ui;
}
