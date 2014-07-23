#include "averagingsettingswidget.h"

#include "../averaging.h"

#include <QGridLayout>
#include <QSpinBox>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace AveragingPlugin;


AveragingSettingsWidget::AveragingSettingsWidget(Averaging *toolbox, QWidget *parent)
: QWidget(parent)
, m_pAveragingToolbox(toolbox)
{
    this->setWindowTitle("Averaging Settings");

    QGridLayout* t_pGridLayout = new QGridLayout;

    m_pSpinBoxPreStimSamples = new QSpinBox;
    m_pSpinBoxPreStimSamples->setMinimum(1);
    m_pSpinBoxPreStimSamples->setMaximum(10000);
    m_pSpinBoxPreStimSamples->setSingleStep(1);
    m_pSpinBoxPreStimSamples->setValue(m_pAveragingToolbox->m_iPreStimSamples);
//    connect(m_pSpinBoxPreStimSamples, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &Averaging::preStimChanged);
    t_pGridLayout->addWidget(m_pSpinBoxPreStimSamples,0,1,1,1);
//    addPluginWidget(m_pSpinBoxPreStimSamples);

    m_pSpinBoxPostStimSamples = new QSpinBox;
    m_pSpinBoxPostStimSamples->setMinimum(1);
    m_pSpinBoxPostStimSamples->setMaximum(10000);
    m_pSpinBoxPostStimSamples->setSingleStep(1);
    m_pSpinBoxPostStimSamples->setValue(m_pAveragingToolbox->m_iPostStimSamples);
//    connect(m_pSpinBoxPostStimSamples, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &Averaging::postStimChanged);
    t_pGridLayout->addWidget(m_pSpinBoxPostStimSamples,1,1,1,1);
//    addPluginWidget(m_pSpinBoxPostStimSamples);

    this->setLayout(t_pGridLayout);
}
