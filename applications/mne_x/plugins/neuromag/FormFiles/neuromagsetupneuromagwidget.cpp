#include "neuromagsetupneuromagwidget.h"
#include "ui_neuromagsetupneuromag.h"


using namespace MneRtClientPlugin;

NeuromagSetupNeuromagWidget::NeuromagSetupNeuromagWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NeuromagSetupNeuromagWidget)
{
    ui->setupUi(this);
}

NeuromagSetupNeuromagWidget::~NeuromagSetupNeuromagWidget()
{
    delete ui;
}
