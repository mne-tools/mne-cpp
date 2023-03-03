#include "signalgensetupwidget.h"
#include "ui_signalgensetupwidget.h"

SignalGenSetupWidget::SignalGenSetupWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SignalGenSetupWidget)
{
    ui->setupUi(this);

    connect(ui->nchannels_spinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SignalGenSetupWidget::numChannelsChanged, Qt::UniqueConnection);

    connect(ui->samplefreq_spinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SignalGenSetupWidget::sampleFreqChanged, Qt::UniqueConnection);
}

SignalGenSetupWidget::~SignalGenSetupWidget()
{
    delete ui;
}

void SignalGenSetupWidget::setConfig(const Config& config)
{
    ui->nchannels_spinBox->setMaximum(config.maxChannels);
    ui->nchannels_spinBox->setMinimum(config.minChannels);
    ui->nchannels_spinBox->setValue(config.defChannels);

    ui->samplefreq_spinBox->setMaximum(config.maxSampFreq);
    ui->samplefreq_spinBox->setMinimum(config.minSampFreq);
    ui->samplefreq_spinBox->setValue(config.defSampFreq);
}

