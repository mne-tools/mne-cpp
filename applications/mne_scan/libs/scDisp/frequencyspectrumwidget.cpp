//=============================================================================================================
/**
* @file     frequencyspectrumwidget.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Limin Sun <liminsun@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Definition of the FrequencySpectrumWidget Class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "frequencyspectrumwidget.h"

#include <disp/viewers/spectrumsettingsview.h>
#include <disp/viewers/spectrumview.h>
#include <scMeas/frequencyspectrum.h>
#include <math.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVBoxLayout>
#include <QSettings>
#include <QSlider>
#include <QAction>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCDISPLIB;
using namespace SCMEASLIB;
using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FrequencySpectrumWidget::FrequencySpectrumWidget(QSharedPointer<FrequencySpectrum> pFS,
                                                 QSharedPointer<QTime> &pTime,
                                                 QWidget* parent)
: MeasurementWidget(parent)
, m_pFS(pFS)
, m_fLowerFrqBound(0)
, m_fUpperFrqBound(300)
, m_bInitialized(false)
{
    Q_UNUSED(pTime)

    m_pActionFrequencySettings = new QAction(QIcon(":/images/frqResolution.png"), tr("Shows the frequency spectrum settings widget (F12)"),this);
    m_pActionFrequencySettings->setShortcut(tr("F12"));
    m_pActionFrequencySettings->setStatusTip(tr("Shows the frequency spectrum settings widget (F12)"));
    connect(m_pActionFrequencySettings, &QAction::triggered,
            this, &FrequencySpectrumWidget::showSpectrumSettingsView);
    addDisplayAction(m_pActionFrequencySettings);

    m_pActionFrequencySettings->setVisible(false);

    m_pSpectrumView = new SpectrumView(this);

    //set vertical layout
    QVBoxLayout *neLayout = new QVBoxLayout(this);
    neLayout->addWidget(m_pSpectrumView);

    //set layouts
    this->setLayout(neLayout);

    getData();
}


//*************************************************************************************************************

FrequencySpectrumWidget::~FrequencySpectrumWidget()
{
    // Store Settings
    if(!m_pFS->getName().isEmpty())  {
        QString t_sFSName = m_pFS->getName();

        QSettings settings;

        settings.setValue(QString("FSW/%1/lowerFrqBound").arg(t_sFSName), m_fLowerFrqBound);
        settings.setValue(QString("FSW/%1/upperFrqBound").arg(t_sFSName), m_fUpperFrqBound);
    }
}


//*************************************************************************************************************

void FrequencySpectrumWidget::update(SCMEASLIB::Measurement::SPtr)
{
    getData();
}


//*************************************************************************************************************

void FrequencySpectrumWidget::getData()
{
    if(!m_bInitialized)
    {
        if(m_pFS->isInit())
        {
            init();

            m_pSpectrumView->addData(m_pFS->getValue());

            initSettingsWidget();
        }
    } else {
        m_pSpectrumView->addData(m_pFS->getValue());
    }
}


//*************************************************************************************************************

void FrequencySpectrumWidget::init()
{
    if(m_pFS->getFiffInfo()) {
        QSettings settings;
        if(!m_pFS->getName().isEmpty()) {
            QString t_sFSName = m_pFS->getName();
            m_fLowerFrqBound = settings.value(QString("FSW/%1/lowerFrqBound").arg(t_sFSName), 0).toFloat();
            m_fUpperFrqBound = settings.value(QString("FSW/%1/upperFrqBound").arg(t_sFSName), 300).toFloat();
        }

        m_pActionFrequencySettings->setVisible(true);

        m_pSpectrumView->init(m_pFS->getFiffInfo(), m_pFS->getScaleType());

        m_bInitialized = true;
    }
}


//*************************************************************************************************************

void FrequencySpectrumWidget::initSettingsWidget()
{
    if(!m_pSpectrumSettingsView) {
        m_pSpectrumSettingsView = QSharedPointer<SpectrumSettingsView>(new SpectrumSettingsView(this));

        m_pSpectrumSettingsView->setWindowTitle("Frequency Spectrum Settings");

        connect(m_pSpectrumSettingsView.data(), &SpectrumSettingsView::settingsChanged, this, &FrequencySpectrumWidget::broadcastSettings);
    }

    if(m_pFS->isInit() && m_pFS->getFiffInfo())
    {
        m_fUpperFrqBound = m_fLowerFrqBound < m_fUpperFrqBound ? m_fUpperFrqBound : m_fLowerFrqBound;
        m_pSpectrumSettingsView->m_pSliderLowerBound->setMinimum(0);
        m_pSpectrumSettingsView->m_pSliderLowerBound->setMaximum((qint32)(m_pFS->getFiffInfo()->sfreq/2)*1000);
        m_pSpectrumSettingsView->m_pSliderLowerBound->setValue((qint32)(m_fLowerFrqBound*1000));

        m_pSpectrumSettingsView->m_pSliderUpperBound->setMinimum(0);
        m_pSpectrumSettingsView->m_pSliderUpperBound->setMaximum((qint32)(m_pFS->getFiffInfo()->sfreq/2)*1000);
        m_pSpectrumSettingsView->m_pSliderUpperBound->setValue((qint32)(m_fUpperFrqBound*1000));
    }

}


//*************************************************************************************************************

void FrequencySpectrumWidget::broadcastSettings()
{
    if(m_pSpectrumSettingsView)
    {
        m_fLowerFrqBound = m_pSpectrumSettingsView->m_pSliderLowerBound->value()/1000.0f;
        m_fUpperFrqBound = m_pSpectrumSettingsView->m_pSliderUpperBound->value()/1000.0f;
        m_pSpectrumView->setBoundaries(m_fLowerFrqBound,m_fUpperFrqBound);
    }
}


//*************************************************************************************************************

void FrequencySpectrumWidget::showSpectrumSettingsView()
{
    initSettingsWidget();
    m_pSpectrumSettingsView->show();
}


//*************************************************************************************************************

bool FrequencySpectrumWidget::eventFilter(QObject *object, QEvent *event)
{
    return QWidget::eventFilter(object, event);
}

