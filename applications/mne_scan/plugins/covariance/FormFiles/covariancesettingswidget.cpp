#include "covariancesettingswidget.h"

#include "../covariance.h"

#include <QGridLayout>
#include <QSpinBox>
#include <QLabel>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace COVARIANCEPLUGIN;


CovarianceSettingsWidget::CovarianceSettingsWidget(Covariance *toolbox, QWidget *parent)
: QWidget(parent)
, m_pCovarianceToolbox(toolbox)
{
    this->setWindowTitle("Covariance Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);

    QGridLayout* t_pGridLayout = new QGridLayout;

//    if(m_pCovarianceToolbox->m_pFiffInfo)
//    {
    QLabel* t_pLabelNumSamples = new QLabel;
    t_pLabelNumSamples->setText("Number of Samples");
    t_pGridLayout->addWidget(t_pLabelNumSamples,0,0,1,1);

    qint32 minSamples = 600;//(qint32)m_pCovarianceToolbox->m_pFiffInfo->sfreq;

    m_pSpinBoxNumSamples = new QSpinBox;
    m_pSpinBoxNumSamples->setMinimum(minSamples);
    m_pSpinBoxNumSamples->setMaximum(minSamples*60);
    m_pSpinBoxNumSamples->setSingleStep(minSamples);
    m_pSpinBoxNumSamples->setValue(toolbox->m_iEstimationSamples);
    connect(m_pSpinBoxNumSamples, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            m_pCovarianceToolbox, &Covariance::changeSamples);
    t_pGridLayout->addWidget(m_pSpinBoxNumSamples,0,1,1,1);
//    }
    this->setLayout(t_pGridLayout);
}
