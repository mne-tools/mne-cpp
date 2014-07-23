#include "averagingsettingswidget.h"

#include "../averaging.h"

#include <QGridLayout>
#include <QSpinBox>
#include <QLabel>
#include <QCheckBox>


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

    QLabel* t_pLabelPreStim = new QLabel;
    t_pLabelPreStim->setText("Pre-Stimulus Samples");
    t_pGridLayout->addWidget(t_pLabelPreStim,0,0,1,2);

    m_pSpinBoxPreStimSamples = new QSpinBox;
    m_pSpinBoxPreStimSamples->setMinimum(1);
    m_pSpinBoxPreStimSamples->setMaximum(10000);
    m_pSpinBoxPreStimSamples->setSingleStep(1);
    m_pSpinBoxPreStimSamples->setValue(m_pAveragingToolbox->m_iPreStimSamples);
//    connect(m_pSpinBoxPreStimSamples, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &Averaging::preStimChanged);
    t_pGridLayout->addWidget(m_pSpinBoxPreStimSamples,0,2,1,1);

    QLabel* t_pLabelPostStim = new QLabel;
    t_pLabelPostStim->setText("Post-Stimulus Samples");
    t_pGridLayout->addWidget(t_pLabelPostStim,1,0,1,2);

    m_pSpinBoxPostStimSamples = new QSpinBox;
    m_pSpinBoxPostStimSamples->setMinimum(1);
    m_pSpinBoxPostStimSamples->setMaximum(10000);
    m_pSpinBoxPostStimSamples->setSingleStep(1);
    m_pSpinBoxPostStimSamples->setValue(m_pAveragingToolbox->m_iPostStimSamples);
//    connect(m_pSpinBoxPostStimSamples, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &Averaging::postStimChanged);
    t_pGridLayout->addWidget(m_pSpinBoxPostStimSamples,1,2,1,1);
//    addPluginWidget(m_pSpinBoxPostStimSamples);

    if(toolbox->m_qListModalities.size() > 0)
    {
        QLabel* t_pLabelModalities = new QLabel;
        t_pLabelModalities->setText("Modalities");
        QFont font;font.setBold(true);
        t_pLabelModalities->setFont(font);
        t_pGridLayout->addWidget(t_pLabelModalities,2,0,1,3);

        for(qint32 i = 0; i < toolbox->m_qListModalities.size(); ++i)
        {
            QLabel* t_pLabelModality = new QLabel;
            t_pLabelModality->setText(toolbox->m_qListModalities[i].first);
            t_pGridLayout->addWidget(t_pLabelModality,3+i,0,1,1);

            QCheckBox* t_pCheckBoxModality = new QCheckBox;
            t_pCheckBoxModality->setChecked(toolbox->m_qListModalities[i].second);
            t_pGridLayout->addWidget(t_pCheckBoxModality,3+i,1,1,1);
        }
    }

    this->setLayout(t_pGridLayout);
}
