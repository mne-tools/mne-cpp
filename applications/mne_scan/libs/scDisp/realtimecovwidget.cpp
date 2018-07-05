//=============================================================================================================
/**
* @file     realtimecovwidget.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
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
* @brief    Definition of the RealTimeCovWidget Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimecovwidget.h"

#include <scMeas/realtimecov.h>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QLabel>
#include <QFont>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCDISPLIB;
using namespace SCMEASLIB;


//=============================================================================================================
/**
* Tool enumeration.
*/
enum Tool
{
    Freeze     = 0,     /**< Freezing tool. */
    Annotation = 1      /**< Annotation tool. */
};


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeCovWidget::RealTimeCovWidget(QSharedPointer<RealTimeCov> pRTC, QSharedPointer<QTime> &pTime, QWidget* parent)
: MeasurementWidget(parent)
, m_pRTC(pRTC)
, m_bInitialized(false)
{
    Q_UNUSED(pTime)

    m_pActionSelectModality = new QAction(QIcon(":/images/covarianceSelection.png"), tr("Shows the covariance modality selection widget (F12)"),this);
    m_pActionSelectModality->setShortcut(tr("F12"));
    m_pActionSelectModality->setStatusTip(tr("Shows the covariance modality selection widget (F12)"));
    connect(m_pActionSelectModality, &QAction::triggered, this, &RealTimeCovWidget::showModalitySelectionWidget);
    addDisplayAction(m_pActionSelectModality);

    //set vertical layout
    m_pRtcLayout = new QVBoxLayout(this);

    m_pLabelInit= new QLabel;
    m_pLabelInit->setText("Acquiring Data");
    m_pLabelInit->setAlignment(Qt::AlignCenter);
    QFont font;font.setBold(true);font.setPointSize(20);
    m_pLabelInit->setFont(font);
    m_pRtcLayout->addWidget(m_pLabelInit);

    m_pImageSc = new ImageSc;
    m_pRtcLayout->addWidget(m_pImageSc);

    //set layouts
    this->setLayout(m_pRtcLayout);

    m_qListPickTypes << "EEG";// << "MEG";

    getData();

}


//*************************************************************************************************************

RealTimeCovWidget::~RealTimeCovWidget()
{

}


//*************************************************************************************************************

void RealTimeCovWidget::update(SCMEASLIB::NewMeasurement::SPtr)
{
    getData();
}


//*************************************************************************************************************

void RealTimeCovWidget::getData()
{
    if(!m_bInitialized || m_pRTC->getValue()->names.size() != m_qListChNames.size())
        if(m_pRTC->isInitialized())
            init();

    if(m_bInitialized)
    {
        MatrixXd data = (m_matSelectorT * m_pRTC->getValue()->data) * m_matSelector;
        m_pImageSc->updateData(data);
    }
}


//*************************************************************************************************************

void RealTimeCovWidget::init()
{
    if(m_pRTC->getValue()->names.size() > 0)
    {
        m_pRtcLayout->removeWidget(m_pLabelInit);
        m_pLabelInit->hide();

        m_pImageSc->setTitle(m_pRTC->getName());

        m_qListChNames = m_pRTC->getValue()->names;

        QList<qint32> qListSelChannel;
        for(qint32 i = 0; i < m_qListChNames.size(); ++i)
        {
            foreach (const QString &type, m_qListPickTypes) {
                if (m_qListChNames[i].contains(type))
                    qListSelChannel.append(i);
            }
        }

        m_matSelector = MatrixXd::Zero(m_pRTC->getValue()->data.cols(), qListSelChannel.size());

        for(qint32 i = 0; i  < qListSelChannel.size(); ++i)
            m_matSelector(qListSelChannel[i],i) = 1;

        m_matSelectorT = m_matSelector.transpose();

        m_bInitialized = true;
    }
}


//*************************************************************************************************************

void RealTimeCovWidget::showModalitySelectionWidget()
{
    if(!m_pModalitySelectionWidget)
    {
        m_pModalitySelectionWidget = QSharedPointer<CovModalityWidget>(new CovModalityWidget(this));

        m_pModalitySelectionWidget->setWindowTitle("Modality Selection");
    }
    m_pModalitySelectionWidget->show();
}
