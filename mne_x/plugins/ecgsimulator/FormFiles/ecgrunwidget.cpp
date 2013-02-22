//=============================================================================================================
/**
* @file		ecgrunwidget.cpp
* @author	Christoph Dinh <christoph.dinh@live.de>;
* @version	1.0
* @date		October, 2010
*
* @section	LICENSE
*
* Copyright (C) 2010 Christoph Dinh. All rights reserved.
*
* No part of this program may be photocopied, reproduced,
* or translated to another program language without the
* prior written consent of the author.
*
*
* @brief	Contains the implementation of the ECGRunWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ecgrunwidget.h"
#include "ecgaboutwidget.h"
#include "../ecgsimulator.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace ECGSimulatorModule;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ECGRunWidget::ECGRunWidget(ECGSimulator *simulator, QWidget *parent)
: QWidget(parent)
, m_pECGSimulator(simulator)
{
    ui.setupUi(this);

    connect(ui.m_qPushButton_About, SIGNAL(released()), this, SLOT(showAboutDialog()));

    ui.m_qTextBrowser_Information->insertHtml(QString("Sampling Rate: %1sps").arg(m_pECGSimulator->m_fSamplingRate));
    ui.m_qTextBrowser_Information->insertHtml(QString("<br>Downsampling Factor: %1x").arg(m_pECGSimulator->m_iDownsamplingFactor));
    ui.m_qTextBrowser_Information->insertHtml(QString("<br><u>ECG I</u>"
    		"<br>Enabled: ") + QString(m_pECGSimulator->m_pECGChannel_ECG_I->isEnabled()?"True":"False") +
    		QString("<br>Visible: ") + QString(m_pECGSimulator->m_pECGChannel_ECG_I->isVisible()?"True":"False") +
    		QString("<br>File: ") + m_pECGSimulator->m_pECGChannel_ECG_I->getChannelFile() +
    		QString("<br>Simulation Signal Length: %1s").arg(m_pECGSimulator->m_pECGChannel_ECG_I->getSamples().size()/m_pECGSimulator->m_fSamplingRate));
    ui.m_qTextBrowser_Information->insertHtml(QString("<br><u>ECG II</u>"
    		"<br>Enabled: ") + QString(m_pECGSimulator->m_pECGChannel_ECG_II->isEnabled()?"True":"False") +
    		QString("<br>Visible: ") + QString(m_pECGSimulator->m_pECGChannel_ECG_II->isVisible()?"True":"False") +
    		QString("<br>File: ") + m_pECGSimulator->m_pECGChannel_ECG_II->getChannelFile() +
    		QString("<br>Simulation Signal Length: %1s").arg(m_pECGSimulator->m_pECGChannel_ECG_II->getSamples().size()/m_pECGSimulator->m_fSamplingRate));
    ui.m_qTextBrowser_Information->insertHtml(QString("<br><u>ECG III</u>"
    		"<br>Enabled: ") + QString(m_pECGSimulator->m_pECGChannel_ECG_III->isEnabled()?"True":"False") +
    		QString("<br>Visible: ") + QString(m_pECGSimulator->m_pECGChannel_ECG_III->isVisible()?"True":"False") +
    		QString("<br>File: ") + m_pECGSimulator->m_pECGChannel_ECG_III->getChannelFile() +
    		QString("<br>Simulation Signal Length: %1s").arg(m_pECGSimulator->m_pECGChannel_ECG_III->getSamples().size()/m_pECGSimulator->m_fSamplingRate));
}


//*************************************************************************************************************

ECGRunWidget::~ECGRunWidget()
{

}


//*************************************************************************************************************

void ECGRunWidget::showAboutDialog()
{
    ECGAboutWidget aboutDialog(this);
    aboutDialog.exec();
}
