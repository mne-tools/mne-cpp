#include "signalgensetupwidget.h"
#include "ui_signalgensetupwidget.h"

SignalGenSetupWidget::SignalGenSetupWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SignalGenSetupWidget)
{
    ui->setupUi(this);
}

SignalGenSetupWidget::~SignalGenSetupWidget()
{
    delete ui;
}

void SignalGenSetupWidget::setConfig(const Config& config)
{

}

void SignalGenSetupWidget::numChannelsChanged(int numChannels)
{

}

void SignalGenSetupWidget::sampleFreqChanged(int sampleFreq)
{

}
