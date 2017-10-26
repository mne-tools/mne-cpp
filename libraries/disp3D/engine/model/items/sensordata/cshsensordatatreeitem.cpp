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
#include "../common/metatreeitem.h"

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

CshSensorDataTreeItem::CshSensorDataTreeItem(int iType, const QString &text)
    : AbstractTreeItem(iType,text)
    , m_bIsDataInit(false)
{
    initItem();
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

void CshSensorDataTreeItem::init(const MatrixX3f &matSurfaceVertColor, const MNEBemSurface &bemSurface, const FiffInfo &fiffInfo, const QString &sSensorType, const double dCancelDist, const QString &sInterpolationFunction)
{
    //@TODO implement this

}

void CshSensorDataTreeItem::addData(const MatrixXd &tSensorData)
{
    //@TODO implement this
}

void CshSensorDataTreeItem::setLoopState(bool bState)
{
    QList<QStandardItem*> lItems = this->findChildren(MetaTreeItemTypes::LoopedStreaming);

    for(int i = 0; i < lItems.size(); i++) {
        if(MetaTreeItem* pAbstractItem = dynamic_cast<MetaTreeItem*>(lItems.at(i))) {
            pAbstractItem->setCheckState(bState == true ? Qt::Checked : Qt::Unchecked);
            QVariant data;
            data.setValue(bState);
            pAbstractItem->setData(data, MetaTreeItemRoles::LoopedStreaming);
        }
    }
}

void CshSensorDataTreeItem::setStreamingActive(bool bState)
{
    QList<QStandardItem*> lItems = this->findChildren(MetaTreeItemTypes::StreamStatus);

    for(int i = 0; i < lItems.size(); i++) {
        if(MetaTreeItem* pAbstractItem = dynamic_cast<MetaTreeItem*>(lItems.at(i))) {
            pAbstractItem->setCheckState(bState == true ? Qt::Checked : Qt::Unchecked);
            QVariant data;
            data.setValue(bState);
            pAbstractItem->setData(data, MetaTreeItemRoles::StreamStatus);
        }
    }
}

void CshSensorDataTreeItem::setTimeInterval(int iMSec)
{
    QList<QStandardItem*> lItems = this->findChildren(MetaTreeItemTypes::StreamingTimeInterval);

    for(int i = 0; i < lItems.size(); i++) {
        if(MetaTreeItem* pAbstractItem = dynamic_cast<MetaTreeItem*>(lItems.at(i))) {
            QVariant data;
            data.setValue(iMSec);
            pAbstractItem->setData(data, MetaTreeItemRoles::StreamingTimeInterval);
            pAbstractItem->setData(data, Qt::DisplayRole);
        }
    }
}

void CshSensorDataTreeItem::setNumberAverages(int iNumberAverages)
{
    QList<QStandardItem*> lItems = this->findChildren(MetaTreeItemTypes::NumberAverages);

    for(int i = 0; i < lItems.size(); i++) {
        if(MetaTreeItem* pAbstractItem = dynamic_cast<MetaTreeItem*>(lItems.at(i))) {
            QVariant data;
            data.setValue(iNumberAverages);
            pAbstractItem->setData(data, MetaTreeItemRoles::NumberAverages);
            pAbstractItem->setData(data, Qt::DisplayRole);
        }
    }
}

void CshSensorDataTreeItem::setColortable(const QString &sColortable)
{
    QList<QStandardItem*> lItems = this->findChildren(MetaTreeItemTypes::ColormapType);

    for(int i = 0; i < lItems.size(); i++) {
        if(MetaTreeItem* pAbstractItem = dynamic_cast<MetaTreeItem*>(lItems.at(i))) {
            QVariant data;
            data.setValue(sColortable);
            pAbstractItem->setData(data, MetaTreeItemRoles::ColormapType);
            pAbstractItem->setData(data, Qt::DisplayRole);
        }
    }
}

void CshSensorDataTreeItem::setNormalization(const QVector3D &vecThresholds)
{
    QList<QStandardItem*> lItems = this->findChildren(MetaTreeItemTypes::DistributedSourceLocThreshold);

    for(int i = 0; i < lItems.size(); i++) {
        if(MetaTreeItem* pAbstractItem = dynamic_cast<MetaTreeItem*>(lItems.at(i))) {
            QVariant data;
            data.setValue(vecThresholds);
            pAbstractItem->setData(data, MetaTreeItemRoles::DistributedSourceLocThreshold);

            QString sTemp = QString("%1,%2,%3").arg(vecThresholds.x()).arg(vecThresholds.y()).arg(vecThresholds.z());
            data.setValue(sTemp);
            pAbstractItem->setData(data, Qt::DisplayRole);
        }
    }
}

void CshSensorDataTreeItem::setCancelDistance(double dCancelDist)
{
    QList<QStandardItem*> lItems = this->findChildren(MetaTreeItemTypes::CancelDistance);

    for(int i = 0; i < lItems.size(); i++) {
        if(MetaTreeItem* pAbstractItem = dynamic_cast<MetaTreeItem*>(lItems.at(i))) {
            QVariant data;
            data.setValue(dCancelDist);
            pAbstractItem->setData(data, MetaTreeItemRoles::CancelDistance);
            pAbstractItem->setData(data, Qt::DisplayRole);
        }
    }
}

void CshSensorDataTreeItem::setInterpolationFunction(const QString &sInterpolationFunction)
{
    QList<QStandardItem*> lItems = this->findChildren(MetaTreeItemTypes::InterpolationFunction);

    for(int i = 0; i < lItems.size(); i++) {
        if(MetaTreeItem* pAbstractItem = dynamic_cast<MetaTreeItem*>(lItems.at(i))) {
            QVariant data;
            data.setValue(sInterpolationFunction);
            pAbstractItem->setData(data, MetaTreeItemRoles::InterpolationFunction);
            pAbstractItem->setData(data, Qt::DisplayRole);
        }
    }
}

void CshSensorDataTreeItem::setSFreq(const double dSFreq)
{
    if(m_pSensorRtDataWorker) {
        m_pSensorRtDataWorker->setSFreq(dSFreq);
    }
}

void CshSensorDataTreeItem::updateBadChannels(const FIFFLIB::FiffInfo &info)
{
    //@TODO implement this
}

void CshSensorDataTreeItem::initItem()
{
    this->setEditable(false);
    this->setToolTip("SensorData item");

    //Add items
    QList<QStandardItem*> list;
    QVariant data;

    MetaTreeItem* pItemStreamStatus = new MetaTreeItem(MetaTreeItemTypes::StreamStatus, "Stream data on/off");
    connect(pItemStreamStatus, &MetaTreeItem::checkStateChanged,
            this, &CshSensorDataTreeItem::onCheckStateWorkerChanged);
    list << pItemStreamStatus;
    list << new QStandardItem(pItemStreamStatus->toolTip());
    this->appendRow(list);
    pItemStreamStatus->setCheckable(true);
    pItemStreamStatus->setCheckState(Qt::Unchecked);

    data.setValue(false);
    pItemStreamStatus->setData(data, MetaTreeItemRoles::StreamStatus);

    MetaTreeItem* pItemColormapType = new MetaTreeItem(MetaTreeItemTypes::ColormapType, "Hot");
    connect(pItemColormapType, &MetaTreeItem::dataChanged,
            this, &CshSensorDataTreeItem::onColormapTypeChanged);
    list.clear();
    list << pItemColormapType;
    list << new QStandardItem(pItemColormapType->toolTip());
    this->appendRow(list);
    data.setValue(QString("Hot"));
    pItemColormapType->setData(data, MetaTreeItemRoles::ColormapType);

    MetaTreeItem* pItemSourceLocNormValue = new MetaTreeItem(MetaTreeItemTypes::DistributedSourceLocThreshold, "0.0, 0.5,10.0");
    connect(pItemSourceLocNormValue, &MetaTreeItem::dataChanged,
            this, &CshSensorDataTreeItem::onDataNormalizationValueChanged);
    list.clear();
    list << pItemSourceLocNormValue;
    list << new QStandardItem(pItemSourceLocNormValue->toolTip());
    this->appendRow(list);
    data.setValue(QVector3D(0.0,5.5,15));
    pItemSourceLocNormValue->setData(data, MetaTreeItemRoles::DistributedSourceLocThreshold);

    MetaTreeItem *pItemStreamingInterval = new MetaTreeItem(MetaTreeItemTypes::StreamingTimeInterval, "17");
    connect(pItemStreamingInterval, &MetaTreeItem::dataChanged,
            this, &CshSensorDataTreeItem::onTimeIntervalChanged);
    list.clear();
    list << pItemStreamingInterval;
    list << new QStandardItem(pItemStreamingInterval->toolTip());
    this->appendRow(list);
    data.setValue(17);
    pItemStreamingInterval->setData(data, MetaTreeItemRoles::StreamingTimeInterval);

    MetaTreeItem *pItemLoopedStreaming = new MetaTreeItem(MetaTreeItemTypes::LoopedStreaming, "Looping on/off");
    connect(pItemLoopedStreaming, &MetaTreeItem::checkStateChanged,
            this, &CshSensorDataTreeItem::onCheckStateLoopedStateChanged);
    pItemLoopedStreaming->setCheckable(true);
    pItemLoopedStreaming->setCheckState(Qt::Checked);
    list.clear();
    list << pItemLoopedStreaming;
    list << new QStandardItem(pItemLoopedStreaming->toolTip());
    this->appendRow(list);

    MetaTreeItem *pItemAveragedStreaming = new MetaTreeItem(MetaTreeItemTypes::NumberAverages, "1");
    connect(pItemAveragedStreaming, &MetaTreeItem::dataChanged,
            this, &CshSensorDataTreeItem::onNumberAveragesChanged);
    list.clear();
    list << pItemAveragedStreaming;
    list << new QStandardItem(pItemAveragedStreaming->toolTip());
    this->appendRow(list);
    data.setValue(1);
    pItemAveragedStreaming->setData(data, MetaTreeItemRoles::NumberAverages);

    MetaTreeItem *pItemCancelDistance = new MetaTreeItem(MetaTreeItemTypes::CancelDistance, "0.05");
    connect(pItemCancelDistance, &MetaTreeItem::dataChanged,
            this, &CshSensorDataTreeItem::onCancelDistanceChanged);
    list.clear();
    list << pItemCancelDistance;
    list << new QStandardItem(pItemCancelDistance->toolTip());
    this->appendRow(list);
    data.setValue(0.05);
    pItemCancelDistance->setData(data, MetaTreeItemRoles::CancelDistance);

    MetaTreeItem* pInterpolationFunction = new MetaTreeItem(MetaTreeItemTypes::InterpolationFunction, "Cubic");
    connect(pInterpolationFunction, &MetaTreeItem::dataChanged,
            this, &CshSensorDataTreeItem::onInterpolationFunctionChanged);
    list.clear();
    list << pInterpolationFunction;
    list << new QStandardItem(pInterpolationFunction->toolTip());
    this->appendRow(list);
    data.setValue(QString("Cubic"));
    pInterpolationFunction->setData(data, MetaTreeItemRoles::InterpolationFunction);
}

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

void CshSensorDataTreeItem::onNewRtData(const MatrixX3f &sensorData)
{
    //@TODO uncomment this
//    QVariant data;
//    data.setValue(sensorData);
//    emit rtCshSensorValuesChanged(data);
}

void CshSensorDataTreeItem::onColormapTypeChanged(const QVariant &sColormapType)
{
    //@TODO implement this
}

void CshSensorDataTreeItem::onTimeIntervalChanged(const QVariant &iMSec)
{
    if(iMSec.canConvert<int>()) {
        if(m_pSensorRtDataWorker) {
            m_pSensorRtDataWorker->setInterval(iMSec.toInt());
        }
    }
}

void CshSensorDataTreeItem::onDataNormalizationValueChanged(const QVariant &vecThresholds)
{
    if(vecThresholds.canConvert<QVector3D>())
    {
        //@TODO implement this
    }
}

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

void CshSensorDataTreeItem::onNumberAveragesChanged(const QVariant &iNumAvr)
{
    if(iNumAvr.canConvert<int>()) {
        if(m_pSensorRtDataWorker) {
            m_pSensorRtDataWorker->setNumberAverages(iNumAvr.toInt());
        }
    }
}

void CshSensorDataTreeItem::onCancelDistanceChanged(const QVariant &dCancelDist)
{
    //@TODO implement this
}

void CshSensorDataTreeItem::onInterpolationFunctionChanged(const QVariant &sInterpolationFunction)
{
    //@TODO implement this
}


//*************************************************************************************************************
