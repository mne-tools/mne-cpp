//=============================================================================================================
/**
* @file     realtimeevokedwidget.cpp
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
* @brief    Implementation of the RealTimeEvokedWidget Class.
*
*/

//ToDo Paint to render area

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimeevokedwidget.h"
//#include "annotationwindow.h"

#include <xMeas/realtimeevoked.h>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <math.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPaintEvent>
#include <QPainter>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QScroller>
#include <QSettings>

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace XDISPLIB;
using namespace XMEASLIB;


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

RealTimeEvokedWidget::RealTimeEvokedWidget(QSharedPointer<RealTimeEvoked> pRTE, QSharedPointer<QTime> &pTime, QWidget* parent)
: NewMeasurementWidget(parent)
, m_pRTEModel(Q_NULLPTR)
, m_pButterflyPlot(Q_NULLPTR)
, m_pRTE(pRTE)
, m_bInitialized(false)
, m_pSensorModel(Q_NULLPTR)
{
    Q_UNUSED(pTime)

    m_pActionSelectModality = new QAction(QIcon(":/images/evokedSettings.png"), tr("Shows the covariance modality selection widget (F12)"),this);
    m_pActionSelectModality->setShortcut(tr("F12"));
    m_pActionSelectModality->setStatusTip(tr("Shows the covariance modality selection widget (F12)"));
    connect(m_pActionSelectModality, &QAction::triggered, this, &RealTimeEvokedWidget::showModalitySelectionWidget);
    addDisplayAction(m_pActionSelectModality);

    m_pActionSelectModality->setVisible(false);

//    m_pActionSelectSensors = new QAction(QIcon(":/images/selectSensors.png"), tr("Shows the region selection widget (F12)"),this);
//    m_pActionSelectSensors->setShortcut(tr("F12"));
//    m_pActionSelectSensors->setStatusTip(tr("Shows the region selection widget (F12)"));
//    m_pActionSelectSensors->setVisible(false);
//    connect(m_pActionSelectSensors, &QAction::triggered, this, &RealTimeEvokedWidget::showSensorSelectionWidget);
//    addDisplayAction(m_pActionSelectSensors);


    //set vertical layout
    m_pRteLayout = new QVBoxLayout(this);

    m_pLabelInit= new QLabel;
    m_pLabelInit->setText("Acquiring Data");
    m_pLabelInit->setAlignment(Qt::AlignCenter);
    QFont font;font.setBold(true);font.setPointSize(20);
    m_pLabelInit->setFont(font);
    m_pRteLayout->addWidget(m_pLabelInit);

    if(m_pButterflyPlot)
        delete m_pButterflyPlot;
    m_pButterflyPlot = new RealTimeButterflyPlot;
    m_pButterflyPlot->hide();

    m_pRteLayout->addWidget(m_pButterflyPlot);



    //set layouts
    this->setLayout(m_pRteLayout);

    getData();
}


//*************************************************************************************************************

RealTimeEvokedWidget::~RealTimeEvokedWidget()
{
    //
    // Store Settings
    //
    if(!m_pRTE->getName().isEmpty())
    {
        QString t_sRTEWName = m_pRTE->getName();

        QSettings settings;

        for(qint32 i = 0; i < m_qListModalities.size(); ++i)
        {
            settings.setValue(QString("RTEW/%1/%2/active").arg(t_sRTEWName).arg(m_qListModalities[i].m_sName), m_qListModalities[i].m_bActive);
            settings.setValue(QString("RTEW/%1/%2/norm").arg(t_sRTEWName).arg(m_qListModalities[i].m_sName), m_qListModalities[i].m_fNorm);
        }
    }


}


//*************************************************************************************************************

void RealTimeEvokedWidget::broadcastSettings()
{
    m_pButterflyPlot->setSettings(m_qListModalities);

}


//*************************************************************************************************************

void RealTimeEvokedWidget::update(XMEASLIB::NewMeasurement::SPtr)
{
    getData();
}


//*************************************************************************************************************

void RealTimeEvokedWidget::getData()
{
    if(!m_bInitialized)
    {
        if(m_pRTE->isInitialized())
        {
//            QFile file(m_pRTE->getXMLLayoutFile());
//            if (!file.open(QFile::ReadOnly | QFile::Text))
//            {
//                qDebug() << QString("Cannot read file %1:\n%2.").arg(m_pRTE->getXMLLayoutFile()).arg(file.errorString());
//                m_pSensorModel = new SensorModel(this);
//                m_pSensorModel->mapChannelInfo(m_qListChInfo);
//            }
//            else
//            {
//                m_pSensorModel = new SensorModel(&file, this);
//                m_pSensorModel->mapChannelInfo(m_qListChInfo);
//                m_pActionSelectSensors->setVisible(true);
//            }

            m_qListChInfo = m_pRTE->chInfo();

            init();

            m_pRTEModel->updateData();
        }
    }
    else
        m_pRTEModel->updateData();
}


//*************************************************************************************************************

void RealTimeEvokedWidget::init()
{
    if(m_pRTE->isInitialized())
    {
        QString t_sRTEWName = m_pRTE->getName();
        qDebug() << "##### NAME #####" << QString("RTEW/%1/MAG/active").arg(t_sRTEWName);
        m_pRteLayout->removeWidget(m_pLabelInit);
        m_pLabelInit->hide();

        m_pButterflyPlot->show();

        if(m_pRTEModel)
            delete m_pRTEModel;
        m_pRTEModel = new RealTimeEvokedModel(this);

        m_pRTEModel->setRTE(m_pRTE);

        m_pButterflyPlot->setModel(m_pRTEModel);

        m_qListModalities.clear();
        bool hasMag = false;
        bool hasGrad = false;
        bool hasEEG = false;
        bool hasEOG = false;
        bool hasMISC = false;
        for(qint32 i = 0; i < m_pRTE->info().nchan; ++i)
        {
            if(m_pRTE->info().chs[i].kind == FIFFV_MEG_CH)
            {
                if(!hasMag && m_pRTE->info().chs[i].unit == FIFF_UNIT_T)
                    hasMag = true;
                else if(!hasGrad &&  m_pRTE->info().chs[i].unit == FIFF_UNIT_T_M)
                    hasGrad = true;
            }
            else if(!hasEEG && m_pRTE->info().chs[i].kind == FIFFV_EEG_CH)
                hasEEG = true;
            else if(!hasEOG && m_pRTE->info().chs[i].kind == FIFFV_EOG_CH)
                hasEOG = true;
            else if(!hasMISC && m_pRTE->info().chs[i].kind == FIFFV_MISC_CH)
                hasMISC = true;
        }
        QSettings settings;
        bool sel = false;
        float val = 1e-11f;
        if(hasMag)
        {
            sel = settings.value(QString("RTEW/%1/MAG/active").arg(t_sRTEWName), true).toBool();
            val = settings.value(QString("RTEW/%1/MAG/norm").arg(t_sRTEWName), 1e-11f).toFloat();
            m_qListModalities.append(Modality("MAG",sel,val));
        }
        if(hasGrad)
        {
            sel = settings.value(QString("RTEW/%1/GRAD/active").arg(t_sRTEWName), true).toBool();
            val = settings.value(QString("RTEW/%1/GRAD/norm").arg(t_sRTEWName), 1e-10f).toFloat();
            m_qListModalities.append(Modality("GRAD",sel,val));
        }
        if(hasEEG)
        {
            sel = settings.value(QString("RTEW/%1/EEG/active").arg(t_sRTEWName), true).toBool();
            val = settings.value(QString("RTEW/%1/EEG/norm").arg(t_sRTEWName), 1e-4f).toFloat();
            m_qListModalities.append(Modality("EEG",sel,val));
        }
        if(hasEOG)
        {
            sel = settings.value(QString("RTEW/%1/EOG/active").arg(t_sRTEWName), true).toBool();
            val = settings.value(QString("RTEW/%1/EOG/norm").arg(t_sRTEWName), 1e-3f).toFloat();
            m_qListModalities.append(Modality("EOG",sel,val));
        }
        if(hasMISC)
        {
            sel = settings.value(QString("RTEW/%1/MISC/active").arg(t_sRTEWName), true).toBool();
            val = settings.value(QString("RTEW/%1/MISC/norm").arg(t_sRTEWName), 1e-3f).toFloat();
            m_qListModalities.append(Modality("MISC",sel,val));
        }

        m_pButterflyPlot->setSettings(m_qListModalities);

        m_pActionSelectModality->setVisible(true);
        // Initialized
        m_bInitialized = true;
    }
}


//*************************************************************************************************************

void RealTimeEvokedWidget::showModalitySelectionWidget()
{
    if(!m_pEvokedModalityWidget)
    {
        m_pEvokedModalityWidget = QSharedPointer<EvokedModalityWidget>(new EvokedModalityWidget(this));

        m_pEvokedModalityWidget->setWindowTitle("Modality Selection");

        connect(m_pEvokedModalityWidget.data(), &EvokedModalityWidget::settingsChanged, this, &RealTimeEvokedWidget::broadcastSettings);
    }
    m_pEvokedModalityWidget->show();
}


//*************************************************************************************************************

void RealTimeEvokedWidget::showSensorSelectionWidget()
{
    if(!m_pSensorSelectionWidget)
    {
        m_pSensorSelectionWidget = QSharedPointer<SensorWidget>(new SensorWidget);

        m_pSensorSelectionWidget->setWindowTitle("Channel Selection");

        if(m_pSensorModel)
        {
            m_pSensorSelectionWidget->setModel(m_pSensorModel);

//            connect(m_pSensorModel, &SensorModel::newSelection, m_pRTMSAModel, &RealTimeMultiSampleArrayModel::selectRows);
        }

    }
    m_pSensorSelectionWidget->show();
}


//*************************************************************************************************************

void RealTimeEvokedWidget::applySelection()
{
//    m_pRTMSAModel->selectRows(m_qListCurrentSelection);

    m_pSensorModel->silentUpdateSelection(m_qListCurrentSelection);
}


//*************************************************************************************************************

void RealTimeEvokedWidget::resetSelection()
{
//    // non C++11 alternative
//    m_qListCurrentSelection.clear();
//    for(qint32 i = 0; i < m_qListChInfo.size(); ++i)
//        m_qListCurrentSelection.append(i);

//    applySelection();
}

