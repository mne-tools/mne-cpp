//=============================================================================================================
/**
* @file     cshsensordatatreeitem.cpp
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
* @brief    CshSensorDataTreeItem class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "cshsensordatatreeitem.h"
#include "../../../../helpers/geometryinfo/geometryinfo.h"
#include "../../../../helpers/interpolation/interpolation.h"
#include "cshinterpolationitem.h"
#include "../../workers/rtSensorData/rtcshsensordataworker.h"


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

typedef double (*dFuncPtr)(double);

dFuncPtr transformInterpolationFromStrToFunc(const QString &tFunctionName)
{
    if(tFunctionName == "Linear") {
        return Interpolation::linear;
    }
    else if(tFunctionName == "Square") {
        return Interpolation::square;
    }
    else if(tFunctionName == "Cubic") {
        return Interpolation::cubic;
    }
    else if(tFunctionName == "Gaussian") {
        return Interpolation::gaussian;
    }
    return nullptr;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CshSensorDataTreeItem::CshSensorDataTreeItem(int iType, const QString &text)
    : SensorDataTreeItem(iType,text)
{
    SensorDataTreeItem::initItem();
}


//*************************************************************************************************************

CshSensorDataTreeItem::~CshSensorDataTreeItem()
{
    if(m_pSensorRtDataWorker->isRunning())
    {
        m_pSensorRtDataWorker->stop();
        delete m_pSensorRtDataWorker;
    }
}


//*************************************************************************************************************

void CshSensorDataTreeItem::init(const MNEBemSurface &tBemSurface,
                                 const FiffInfo &tFiffInfo,
                                 const QString &tSensorType,
                                 const double tCancelDist,
                                 const QString &tInterpolationFunction,
                                 Qt3DCore::QEntity* t3DEntityParent)
{
    if(m_bIsDataInit == true)
    {
        qDebug("CshSensorDataTreeItem::init is already initialized");
    }

    this->setData(0, Data3DTreeModelItemRoles::RTData);

    if(!m_pSensorRtDataWorker) {
        m_pSensorRtDataWorker = new RtCshSensorDataWorker();
    }

    connect(m_pSensorRtDataWorker.data(), &RtCshSensorDataWorker::newRtData,
            this, &CshSensorDataTreeItem::onNewRtData);

    // map passed sensor type string to fiff constant
    if (tSensorType.toStdString() == std::string("MEG")) {
        m_iSensorType = FIFFV_MEG_CH;
    } else if (tSensorType.toStdString() == std::string("EEG")) {
        m_iSensorType = FIFFV_EEG_CH;
    } else {
        qDebug() << "SensorDataTreeItem::init - unknown sensor type. Returning ...";
        return;
    }

    //fill QVector with the right sensor positions
    QVector<Vector3f> vecSensorPos;
    m_iUsedSensors.clear();
    int iCounter = 0;
    for(const FiffChInfo &info : tFiffInfo.chs) {
        //Only take EEG with V as unit or MEG magnetometers with T as unit
        if(info.kind == m_iSensorType && (info.unit == FIFF_UNIT_T || info.unit == FIFF_UNIT_V)) {
            vecSensorPos.push_back(info.chpos.r0);

            //save the number of the sensor
            m_iUsedSensors.push_back(iCounter);
        }
        iCounter++;
    }

    //Create bad channel idx list
    for(const QString &bad : tFiffInfo.bads) {
        m_iSensorsBad.push_back(tFiffInfo.ch_names.indexOf(bad));
    }

    //Set cancle distance
    setCancelDistance(tCancelDist);
    //Set interpolation function
    setInterpolationFunction(tInterpolationFunction);

    //Set surface data
    m_bemSurface = tBemSurface;
    m_fiffInfo = tFiffInfo;

    //sensor projecting
    m_pVecMappedSubset = GeometryInfo::projectSensors(tBemSurface, vecSensorPos);

    //SCDC with cancel distance
    m_pDistanceMatrix = GeometryInfo::scdc(tBemSurface, m_pVecMappedSubset, tCancelDist);

    //filtering of bad channels out of the distance table
    GeometryInfo::filterBadChannels(m_pDistanceMatrix, tFiffInfo, m_iSensorType);

    dFuncPtr interpolationFunc = transformInterpolationFromStrToFunc(tInterpolationFunction);
    //create weight matrix
    QSharedPointer<SparseMatrix<double>> pInterpolationMatrix = Interpolation::createInterpolationMat(m_pVecMappedSubset,
                                                                               m_pDistanceMatrix,
                                                                               interpolationFunc,
                                                                               tCancelDist,
                                                                               tFiffInfo,
                                                                               m_iSensorType);

    //create new Tree Item
    if(!m_pInterpolationItem)
    {
        m_pInterpolationItem = new CshInterpolationItem(t3DEntityParent, Data3DTreeModelItemTypes::CshInterpolationItem, QStringLiteral("3D Plot"));
        m_pInterpolationItem->initData(tBemSurface, pInterpolationMatrix);

        QList<QStandardItem*> list;
        list << m_pInterpolationItem;
        list << new QStandardItem(m_pInterpolationItem->toolTip());
        this->appendRow(list);
    }

    //Init complete
    m_bIsDataInit = true;
}


//*************************************************************************************************************

void CshSensorDataTreeItem::addData(const MatrixXd &tSensorData)
{
    if(!m_bIsDataInit) {
        qDebug() << "CshSensorDataTreeItem::addData - sensor data item has not been initialized yet!";
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
        m_pSensorRtDataWorker->addData(dSmallSensorData.cast<float>());
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
        m_pSensorRtDataWorker->addData(dSmallSensorData.cast<float>());
    }
}


//*************************************************************************************************************

void CshSensorDataTreeItem::setSFreq(const double dSFreq)
{
    if(m_pSensorRtDataWorker) {
        m_pSensorRtDataWorker->setSFreq(dSFreq);
    }
}


//*************************************************************************************************************

void CshSensorDataTreeItem::updateBadChannels(const FIFFLIB::FiffInfo &info)
{
    //Create bad channel idx list
    m_iSensorsBad.clear();
    for(const QString &bad : info.bads)
    {
        m_iSensorsBad.push_back(info.ch_names.indexOf(bad));
    }

    if(!m_bIsDataInit)
    {
        return;
    }


    m_fiffInfo = info;

    //filtering of bad channels out of the distance table
    GeometryInfo::filterBadChannels(m_pDistanceMatrix,
                                    m_fiffInfo,
                                    m_iSensorType);

    //Update weight matrix
    m_pInterpolationItem->setWeightMatrix(Interpolation::createInterpolationMat(m_pVecMappedSubset,
                                                                                m_pDistanceMatrix,
                                                                                m_interpolationFunction,
                                                                                m_dCancelDistance,
                                                                                m_fiffInfo,
                                                                                m_iSensorType));
}


//*************************************************************************************************************

QSharedPointer<SparseMatrix<double>> CshSensorDataTreeItem::calculateWeigtMatrix()
{
    //SCDC with cancel distance
    m_pDistanceMatrix = GeometryInfo::scdc(m_bemSurface,
                                           m_pVecMappedSubset,
                                           m_dCancelDistance);

    //filtering of bad channels out of the distance table
    GeometryInfo::filterBadChannels(m_pDistanceMatrix,
                                    m_fiffInfo,
                                    m_iSensorType);


    //create weight matrix
    return  Interpolation::createInterpolationMat(m_pVecMappedSubset,
                                                   m_pDistanceMatrix,
                                                   m_interpolationFunction,
                                                   m_dCancelDistance,
                                                   m_fiffInfo,
                                                   m_iSensorType);

}


//*************************************************************************************************************

void CshSensorDataTreeItem::onCheckStateWorkerChanged(const Qt::CheckState &checkState)
{
    if(m_pSensorRtDataWorker) {
        if(checkState == Qt::Checked) {
            m_pSensorRtDataWorker->start();
        } else if(checkState == Qt::Unchecked) {
            m_pSensorRtDataWorker->stop();
        }
    }
}


//*************************************************************************************************************

void CshSensorDataTreeItem::onNewRtData(const VectorXf &tSensorData)
{
    if(m_pInterpolationItem)
    {
        m_pInterpolationItem->addNewRtData(tSensorData);

    }
}


//*************************************************************************************************************

void CshSensorDataTreeItem::onColormapTypeChanged(const QVariant &sColormapType)
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

void CshSensorDataTreeItem::onTimeIntervalChanged(const QVariant &iMSec)
{
    if(iMSec.canConvert<int>()) {
        if(m_pSensorRtDataWorker) {
            m_pSensorRtDataWorker->setInterval(iMSec.toInt());
        }
    }
}


//*************************************************************************************************************

void CshSensorDataTreeItem::onDataNormalizationValueChanged(const QVariant &vecThresholds)
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

void CshSensorDataTreeItem::onCheckStateLoopedStateChanged(const Qt::CheckState &checkState)
{
    if(m_pSensorRtDataWorker) {
        if(checkState == Qt::Checked) {
            m_pSensorRtDataWorker->setLoop(true);
        } else if(checkState == Qt::Unchecked) {
            m_pSensorRtDataWorker->setLoop(false);
        }
    }
}


//*************************************************************************************************************

void CshSensorDataTreeItem::onNumberAveragesChanged(const QVariant &iNumAvr)
{
    if(iNumAvr.canConvert<int>()) {
        if(m_pSensorRtDataWorker) {
            m_pSensorRtDataWorker->setNumberAverages(iNumAvr.toInt());
        }
    }
}


//*************************************************************************************************************

void CshSensorDataTreeItem::onCancelDistanceChanged(const QVariant &dCancelDist)
{
    if(dCancelDist.canConvert<double>())
    {
        m_dCancelDistance = dCancelDist.toDouble();

        if(m_pInterpolationItem != nullptr && m_bIsDataInit == true)
        {
            //SCDC with cancel distance
            m_pDistanceMatrix = GeometryInfo::scdc(m_bemSurface,
                                                   m_pVecMappedSubset,
                                                   m_dCancelDistance);

            //filtering of bad channels out of the distance table
            GeometryInfo::filterBadChannels(m_pDistanceMatrix,
                                            m_fiffInfo,
                                            m_iSensorType);


            //create weight matrix
            m_pInterpolationItem->setWeightMatrix(Interpolation::createInterpolationMat(m_pVecMappedSubset,
                                                                                        m_pDistanceMatrix,
                                                                                        m_interpolationFunction,
                                                                                        m_dCancelDistance,
                                                                                        m_fiffInfo,
                                                                                        m_iSensorType));
        }
    }
}


//*************************************************************************************************************

void CshSensorDataTreeItem::onInterpolationFunctionChanged(const QVariant &sInterpolationFunction)
{
    if(sInterpolationFunction.canConvert<QString>())
    {
        m_interpolationFunction = transformInterpolationFromStrToFunc(sInterpolationFunction.toString());

        if(m_pInterpolationItem && m_bIsDataInit == true)
        {
            m_pInterpolationItem->setWeightMatrix(Interpolation::createInterpolationMat(m_pVecMappedSubset,
                                                                                        m_pDistanceMatrix,
                                                                                        m_interpolationFunction,
                                                                                        m_dCancelDistance,
                                                                                        m_fiffInfo,
                                                                                        m_iSensorType));
        }
    }
}


//*************************************************************************************************************
