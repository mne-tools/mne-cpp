//=============================================================================================================
/**
* @file     gpusensordatatreeitem.cpp
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     October, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lars Debor and Matti Hamalainen. All rights reserved.
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
* @brief    GpuSensorDataTreeItem class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "gpusensordatatreeitem.h"
#include "../../../../helpers/geometryinfo/geometryinfo.h"
#include "../../../../helpers/interpolation/interpolation.h"
#include "gpuinterpolationitem.h"
#include "../../workers/rtSensorData/RtSensorDataWorker.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector3D>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

GpuSensorDataTreeItem::GpuSensorDataTreeItem(int iType, const QString &text)
    : SensorDataTreeItem(iType,text)
{
    SensorDataTreeItem::initItem();
}


//*************************************************************************************************************

void GpuSensorDataTreeItem::init(const MNEBemSurface &bemSurface,
                                 const FiffInfo &fiffInfo,
                                 const QString &sSensorType,
                                 const double dCancelDist,
                                 const QString &sInterpolationFunction,
                                 Qt3DCore::QEntity* p3DEntityParent)
{
    if(m_bIsDataInit == true)
    {
        qDebug("GpuSensorDataTreeItem::init - Item is already initialized");
    }

    this->setData(0, Data3DTreeModelItemRoles::RTData);

    if(!m_pSensorRtDataWorkController) {
        m_pSensorRtDataWorkController = new RtSensorDataController(false);

        connect(m_pSensorRtDataWorkController, &RtSensorDataController::newInterpolationMatrixAvailable,
                this, &GpuSensorDataTreeItem::setInterpolationMatrix);
        connect(m_pSensorRtDataWorkController, &RtSensorDataController::newRtRawData,
                this, &GpuSensorDataTreeItem::onNewRtRawData);
    }


    // map passed sensor type string to fiff constant
    fiff_int_t sensorTypeFiffConstant;
    if (sSensorType.toStdString() == std::string("MEG")) {
        sensorTypeFiffConstant = FIFFV_MEG_CH;
    } else if (sSensorType.toStdString() == std::string("EEG")) {
        sensorTypeFiffConstant = FIFFV_EEG_CH;
    } else {
        qDebug() << "GpuSensorDataTreeItem::init - unknown sensor type. Returning ...";
        return;
    }

    //fill QVector with the right sensor positions
    QVector<Vector3f> vecSensorPos;
    m_iUsedSensors.clear();
    int iCounter = 0;
    for(const FiffChInfo &info : fiffInfo.chs) {
        //Only take EEG with V as unit or MEG magnetometers with T as unit
        if(info.kind == sensorTypeFiffConstant && (info.unit == FIFF_UNIT_T || info.unit == FIFF_UNIT_V)) {
            vecSensorPos.push_back(info.chpos.r0);

            //save the number of the sensor
            m_iUsedSensors.push_back(iCounter);
        }
        iCounter++;
    }

    //Create bad channel idx list
    for(const QString &bad : fiffInfo.bads) {
        m_iSensorsBad.push_back(fiffInfo.ch_names.indexOf(bad));
    }

    //Set cancel distance
    setCancelDistance(dCancelDist);

    //Set interpolation function
    setInterpolationFunction(sInterpolationFunction);

    m_pSensorRtDataWorkController->setInterpolationInfo(bemSurface,
                                                vecSensorPos,
                                                fiffInfo,
                                                sensorTypeFiffConstant);

    //create new Tree Item
    if(!m_pInterpolationItem)
    {
        m_pInterpolationItem = new GpuInterpolationItem(p3DEntityParent, Data3DTreeModelItemTypes::GpuInterpolationItem, QStringLiteral("3D Plot"));
        m_pInterpolationItem->initData(bemSurface);

        QList<QStandardItem*> list;
        list << m_pInterpolationItem;
        list << new QStandardItem(m_pInterpolationItem->toolTip());
        this->appendRow(list);
    }

    //Init complete
    m_bIsDataInit = true;
}


//*************************************************************************************************************

void GpuSensorDataTreeItem::addData(const MatrixXd &tSensorData)
{
    if(!m_bIsDataInit) {
        qDebug() << "GpuSensorDataTreeItem::addData - sensor data item has not been initialized yet!";
        return;
    }

    //if more data then needed is provided
    const uint iSensorSize = m_iUsedSensors.size();
    if(tSensorData.rows() > iSensorSize)
    {
        MatrixXd dSmallSensorData(iSensorSize, tSensorData.cols());
        for(uint i = 0 ; i < iSensorSize; ++i)
        {
            //Set bad channels to zero
            if(m_iSensorsBad.contains(m_iUsedSensors[i])) {
                dSmallSensorData.row(i).setZero();
            } else {
                dSmallSensorData.row(i) = tSensorData.row(m_iUsedSensors[i]);
            }
        }
        //Set new data into item's data.
        QVariant data;
        data.setValue(dSmallSensorData);
        this->setData(data, Data3DTreeModelItemRoles::RTData);

        //Add data to worker
        m_pSensorRtDataWorkController->addData(dSmallSensorData);
    }
    else
    {
        //Set bad channels to zero
        MatrixXd dSmallSensorData = tSensorData;
        for(uint i = 0 ; i < dSmallSensorData.rows(); ++i)
        {
            if(m_iSensorsBad.contains(m_iUsedSensors[i])) {
                dSmallSensorData.row(i).setZero();
            }
        }

        //Set new data into item's data.
        QVariant data;
        data.setValue(dSmallSensorData);
        this->setData(data, Data3DTreeModelItemRoles::RTData);

        //Add data to worker
        m_pSensorRtDataWorkController->addData(dSmallSensorData);
    }
}


//*************************************************************************************************************

void GpuSensorDataTreeItem::setSFreq(const double dSFreq)
{
    if(m_pSensorRtDataWorkController) {
        //m_pSensorRtDataWorker->setSFreq(dSFreq);
    }
}


//*************************************************************************************************************

void GpuSensorDataTreeItem::setInterpolationMatrix(QSharedPointer<SparseMatrix<float>> matInterpolationOperator)
{
    m_pInterpolationItem->setWeightMatrix(matInterpolationOperator);
}


//*************************************************************************************************************

void GpuSensorDataTreeItem::updateBadChannels(const FIFFLIB::FiffInfo &info)
{
    if(m_pSensorRtDataWorkController) {
        //Create bad channel idx list
        m_iSensorsBad.clear();
        for(const QString &bad : info.bads) {
            m_iSensorsBad.push_back(info.ch_names.indexOf(bad));
        }

        //qDebug() << "CpuSensorDataTreeItem::updateBadChannels - m_iSensorsBad" << m_iSensorsBad;
        //m_pSensorRtDataWorker->updateBadChannels(info);
    }
}


//*************************************************************************************************************

void GpuSensorDataTreeItem::onStreamingStateChanged(const Qt::CheckState &checkState)
{
    if(m_pSensorRtDataWorkController) {
        if(checkState == Qt::Checked) {
            m_pSensorRtDataWorkController->setStreamingState(true);
        } else if(checkState == Qt::Unchecked) {
            m_pSensorRtDataWorkController->setStreamingState(false);
        }
    }
}


//*************************************************************************************************************

void GpuSensorDataTreeItem::onNewRtRawData(const VectorXd &vecDataVector)
{
    if(m_pInterpolationItem)
    {
        m_pInterpolationItem->addNewRtData(vecDataVector.cast<float>());
    }
}


//*************************************************************************************************************

void GpuSensorDataTreeItem::onColormapTypeChanged(const QVariant &sColormapType)
{
    if(sColormapType.canConvert<QString>())
    {
        if(m_pInterpolationItem)
        {
            m_pInterpolationItem->setColormapType(sColormapType.toString());
        }
    }
}


//*************************************************************************************************************

void GpuSensorDataTreeItem::onTimeIntervalChanged(const QVariant &iMSec)
{
    if(iMSec.canConvert<int>()) {
        if(m_pSensorRtDataWorkController) {
            m_pSensorRtDataWorkController->setTimeInterval(iMSec.toInt());
        }
    }
}


//*************************************************************************************************************

void GpuSensorDataTreeItem::onDataNormalizationValueChanged(const QVariant &vecThresholds)
{
    if(vecThresholds.canConvert<QVector3D>())
    {
        if(m_pInterpolationItem)
        {
            m_pInterpolationItem->setNormalization(vecThresholds.value<QVector3D>());
        }
    }
}


//*************************************************************************************************************

void GpuSensorDataTreeItem::onCheckStateLoopedStateChanged(const Qt::CheckState &checkState)
{
    if(m_pSensorRtDataWorkController) {
        if(checkState == Qt::Checked) {
           //m_pSensorRtDataWorker->setLoop(true);
        } else if(checkState == Qt::Unchecked) {
           //m_pSensorRtDataWorker->setLoop(false);
        }
    }
}


//*************************************************************************************************************

void GpuSensorDataTreeItem::onNumberAveragesChanged(const QVariant &iNumAvr)
{
    if(iNumAvr.canConvert<int>()) {
        if(m_pSensorRtDataWorkController) {
           //m_pSensorRtDataWorker->setNumberAverages(iNumAvr.toInt());
        }
    }
}


//*************************************************************************************************************

void GpuSensorDataTreeItem::onCancelDistanceChanged(const QVariant &dCancelDist)
{
    if(dCancelDist.canConvert<double>())
    {
        if(m_pSensorRtDataWorkController) {
            //m_pSensorRtDataWorker->setCancelDistance(dCancelDist.toDouble());
        }
    }
}


//*************************************************************************************************************

void GpuSensorDataTreeItem::onInterpolationFunctionChanged(const QVariant &sInterpolationFunction)
{
    if(sInterpolationFunction.canConvert<QString>())
    {
        if(m_pSensorRtDataWorkController) {
            m_pSensorRtDataWorkController->setInterpolationFunction(sInterpolationFunction.toString());

            if(m_pInterpolationItem) {
                //m_pInterpolationItem->setWeightMatrix(m_pSensorRtDataWorker->getInterpolationOperator());
            }
        }
    }
}


//*************************************************************************************************************
