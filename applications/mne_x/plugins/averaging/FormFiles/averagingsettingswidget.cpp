#include "averagingsettingswidget.h"

#include "../averaging.h"

#include <QGridLayout>
#include <QSpinBox>
#include <QLabel>


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
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);

    QGridLayout* t_pGridLayout = new QGridLayout;

    if(m_pAveragingToolbox->m_pFiffInfo && m_pAveragingToolbox->m_qListStimChs.size() > 0)
    {
        QLabel* t_pLabelStimChannel = new QLabel;
        t_pLabelStimChannel->setText("Stimulus Channel");
        t_pGridLayout->addWidget(t_pLabelStimChannel,0,0,1,2);

        m_pComboBoxChSelection = new QComboBox;

        for(qint32 i = 0; i < m_pAveragingToolbox->m_qListStimChs.size(); ++i)
        {
            if(m_pAveragingToolbox->m_pFiffInfo->ch_names[ m_pAveragingToolbox->m_qListStimChs[i] ] != QString("STI 014"))
            {
                qDebug() << "Insert" << i << m_pAveragingToolbox->m_pFiffInfo->ch_names[ m_pAveragingToolbox->m_qListStimChs[i] ];
                m_pComboBoxChSelection->insertItem(i,m_pAveragingToolbox->m_pFiffInfo->ch_names[ m_pAveragingToolbox->m_qListStimChs[i] ],QVariant(i));
            }
        }

        m_pComboBoxChSelection->setCurrentIndex(m_pAveragingToolbox->m_iStimChan);

        connect(m_pComboBoxChSelection, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), m_pAveragingToolbox, &Averaging::changeStimChannel);

        t_pGridLayout->addWidget(m_pComboBoxChSelection,0,2,1,1);
    }


    if(m_pAveragingToolbox->m_pRtAve)
    {
        QLabel* t_pLabelNumAve = new QLabel;
        t_pLabelNumAve->setText("Number of Averages");
        t_pGridLayout->addWidget(t_pLabelNumAve,1,0,1,2);

        m_pSpinBoxNumAverages = new QSpinBox;
        m_pSpinBoxNumAverages->setMinimum(1);
        m_pSpinBoxNumAverages->setMaximum(1000);
        m_pSpinBoxNumAverages->setSingleStep(1);
        m_pSpinBoxNumAverages->setValue(m_pAveragingToolbox->m_iNumAverages);
        connect(m_pSpinBoxNumAverages, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), m_pAveragingToolbox, &Averaging::changeNumAverages);
        t_pGridLayout->addWidget(m_pSpinBoxNumAverages,1,2,1,1);
    }

    //Pre Post stimulus
    QLabel* t_pLabelPreStim = new QLabel;
    t_pLabelPreStim->setText("Pre-Stimulus in ms");
    t_pGridLayout->addWidget(t_pLabelPreStim,2,0,1,2);

    m_pSpinBoxPreStimSamples = new QSpinBox;
    m_pSpinBoxPreStimSamples->setMinimum(10);
    m_pSpinBoxPreStimSamples->setMaximum(10000);
    m_pSpinBoxPreStimSamples->setSingleStep(10);
    m_pSpinBoxPreStimSamples->setPrefix("-");
    m_pSpinBoxPreStimSamples->setValue(m_pAveragingToolbox->m_iPreStimSeconds);
    connect(m_pSpinBoxPreStimSamples, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &AveragingSettingsWidget::changePreStim);
    t_pGridLayout->addWidget(m_pSpinBoxPreStimSamples,2,2,1,1);

    QLabel* t_pLabelPostStim = new QLabel;
    t_pLabelPostStim->setText("Post-Stimulus in ms");
    t_pGridLayout->addWidget(t_pLabelPostStim,3,0,1,2);

    m_pSpinBoxPostStimSamples = new QSpinBox;
    m_pSpinBoxPostStimSamples->setMinimum(10);
    m_pSpinBoxPostStimSamples->setMaximum(10000);
    m_pSpinBoxPostStimSamples->setSingleStep(10);
    m_pSpinBoxPostStimSamples->setValue(m_pAveragingToolbox->m_iPostStimSeconds);
    connect(m_pSpinBoxPostStimSamples, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &AveragingSettingsWidget::changePostStim);
    t_pGridLayout->addWidget(m_pSpinBoxPostStimSamples,3,2,1,1);

    //Baseline Correction
    m_pcheckBoxBaselineCorrection = new QCheckBox;
    m_pcheckBoxBaselineCorrection->setText("Use Baseline Correction");
    t_pGridLayout->addWidget(m_pcheckBoxBaselineCorrection,4,0,1,2);
    connect(m_pcheckBoxBaselineCorrection, &QCheckBox::clicked, m_pAveragingToolbox, &Averaging::changeBaselineActive);

    QLabel* t_pLabelBaselineMin = new QLabel;
    t_pLabelBaselineMin->setText("Baseline min in ms");
    t_pGridLayout->addWidget(t_pLabelBaselineMin,5,0,1,2);

    m_pSpinBoxBaselineFrom = new QSpinBox;
    m_pSpinBoxBaselineFrom->setMinimum(m_pSpinBoxPreStimSamples->value()*-1);
    m_pSpinBoxBaselineFrom->setMaximum(m_pSpinBoxPostStimSamples->value());
    m_pSpinBoxBaselineFrom->setSingleStep(10);
    m_pSpinBoxBaselineFrom->setValue(m_pSpinBoxPreStimSamples->value());
    connect(m_pSpinBoxBaselineFrom, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &AveragingSettingsWidget::changeBaselineFrom);
    t_pGridLayout->addWidget(m_pSpinBoxBaselineFrom,5,2,1,1);

    QLabel* t_pLabelBaselineMax = new QLabel;
    t_pLabelBaselineMax->setText("Baseline max in ms");
    t_pGridLayout->addWidget(t_pLabelBaselineMax,6,0,1,2);

    m_pSpinBoxBaselineTo = new QSpinBox;
    m_pSpinBoxBaselineTo->setMinimum(m_pSpinBoxPreStimSamples->value()*-1);
    m_pSpinBoxBaselineTo->setMaximum(m_pSpinBoxPostStimSamples->value());
    m_pSpinBoxBaselineTo->setSingleStep(10);
    m_pSpinBoxBaselineTo->setValue(m_pSpinBoxPreStimSamples->value());
    connect(m_pSpinBoxBaselineTo, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &AveragingSettingsWidget::changeBaselineTo);
    t_pGridLayout->addWidget(m_pSpinBoxBaselineTo,6,2,1,1);

    this->setLayout(t_pGridLayout);
}


//*************************************************************************************************************

void AveragingSettingsWidget::changePreStim(qint32 mSeconds)
{
    m_pSpinBoxBaselineTo->setMinimum(m_pSpinBoxPreStimSamples->value()*-1);
    m_pSpinBoxBaselineFrom->setMinimum(m_pSpinBoxPreStimSamples->value()*-1);

    m_pAveragingToolbox->changePreStim(mSeconds);
}


//*************************************************************************************************************

void AveragingSettingsWidget::changePostStim(qint32 mSeconds)
{
    m_pSpinBoxBaselineTo->setMaximum(m_pSpinBoxPostStimSamples->value());
    m_pSpinBoxBaselineFrom->setMaximum(m_pSpinBoxPostStimSamples->value());

    m_pAveragingToolbox->changePostStim(mSeconds);
}

//*************************************************************************************************************

void AveragingSettingsWidget::changeBaselineFrom(qint32 mSeconds)
{
    m_pSpinBoxBaselineTo->setMinimum(mSeconds);

    m_pAveragingToolbox->changeBaselineFrom(mSeconds/*+m_pSpinBoxPreStimSamples->value()*/);
}


//*************************************************************************************************************

void AveragingSettingsWidget::changeBaselineTo(qint32 mSeconds)
{
    m_pSpinBoxBaselineFrom->setMaximum(mSeconds);

    m_pAveragingToolbox->changeBaselineTo(mSeconds/*+m_pSpinBoxPreStimSamples->value()*/);
}

