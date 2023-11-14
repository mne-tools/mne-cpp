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

    connect(ui->blockpersecond_spinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SignalGenSetupWidget::blocksPerSecondChanged, Qt::UniqueConnection);

    connect(ui->checkBox_random, &QCheckBox::stateChanged,
            [this](int state){if(state == Qt::Checked) this->emit modeChanged(SIGNALGENPLUGIN::SignalGen::Mode::Noise); this->updateUiState();});

    connect(ui->checkBox_wave, &QCheckBox::stateChanged,
            [this](int state){if(state == Qt::Checked) this->emit modeChanged(SIGNALGENPLUGIN::SignalGen::Mode::Wave); this->updateUiState();});

    connect(ui->checkBox_zero, &QCheckBox::stateChanged,
            [this](int state){if(state == Qt::Checked) this->emit modeChanged(SIGNALGENPLUGIN::SignalGen::Mode::Zero); this->updateUiState();});

    connect(ui->spinBox_gen_frq, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SignalGenSetupWidget::genFreqChanged, Qt::UniqueConnection);

    updateUiState();
}

SignalGenSetupWidget::~SignalGenSetupWidget()
{
    delete ui;
}

void SignalGenSetupWidget::defineChannelSettings(int min, int max, int def)
{
    ui->nchannels_spinBox->setMinimum(min);
    ui->nchannels_spinBox->setMaximum(max);
    ui->nchannels_spinBox->setValue(def);
}

void SignalGenSetupWidget::defineSampleFreqSettings(int min, int max, int def)
{
    ui->samplefreq_spinBox->setMinimum(min);
    ui->samplefreq_spinBox->setMaximum(max);
    ui->samplefreq_spinBox->setValue(def);
}

void SignalGenSetupWidget::defineBlockSettings(int min, int max, int def)
{
    ui->blockpersecond_spinBox->setMinimum(min);
    ui->blockpersecond_spinBox->setMaximum(max);
    ui->blockpersecond_spinBox->setValue(def);
}

void SignalGenSetupWidget::defineGeneratedFreqSettings(int min, int max, int def)
{
    ui->spinBox_gen_frq->setMinimum(min);
    ui->spinBox_gen_frq->setMaximum(max);
    ui->spinBox_gen_frq->setValue(def);
}

void SignalGenSetupWidget::defineMode(SIGNALGENPLUGIN::SignalGen::Mode mode)
{
    switch (mode){
    case SIGNALGENPLUGIN::SignalGen::Mode::Noise:
    {
        ui->checkBox_random->setChecked(true);
        break;
    }
    case SIGNALGENPLUGIN::SignalGen::Mode::Wave:
    {
        ui->checkBox_wave->setChecked(true);
        break;
    }
    case SIGNALGENPLUGIN::SignalGen::Mode::Zero:
    {
        ui->checkBox_zero->setChecked(true);
        break;
    }
    }

    updateUiState();
}


void SignalGenSetupWidget::updateUiState()
{
    if(ui->checkBox_wave->isChecked()){
        ui->label_gen_freq->show();
        ui->spinBox_gen_frq->show();
    } else {
        ui->label_gen_freq->hide();
        ui->spinBox_gen_frq->hide();
    }
}
