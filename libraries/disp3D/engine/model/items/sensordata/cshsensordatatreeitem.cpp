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
#include "../../../../helpers/geometryinfo/geometryinfo.h"
#include "../../../../helpers/interpolation/interpolation.h"
#include "cshinterpolationitem.h"

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
}


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
    fiff_int_t sensorTypeFiffConstant;
    if (tSensorType.toStdString() == std::string("MEG")) {
        sensorTypeFiffConstant = FIFFV_MEG_CH;
    } else if (tSensorType.toStdString() == std::string("EEG")) {
        sensorTypeFiffConstant = FIFFV_EEG_CH;
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
        if(info.kind == sensorTypeFiffConstant && (info.unit == FIFF_UNIT_T || info.unit == FIFF_UNIT_V)) {
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

    //sensor projecting
    QSharedPointer<QVector<qint32>> pMappedSubSet = GeometryInfo::projectSensors(tBemSurface, vecSensorPos);

    //SCDC with cancel distance
    QSharedPointer<MatrixXd> pDistanceMatrix = GeometryInfo::scdc(tBemSurface, pMappedSubSet, tCancelDist);

    //filtering of bad channels out of the distance table
    GeometryInfo::filterBadChannels(pDistanceMatrix, tFiffInfo, sensorTypeFiffConstant);

    dFuncPtr interpolationFunc = transformInterpolationFromStrToFunc(tInterpolationFunction);
    //create weight matrix
    QSharedPointer<SparseMatrix<double>> pInterpolationMatrix = Interpolation::createInterpolationMat(pMappedSubSet,
                                                                               pDistanceMatrix,
                                                                               interpolationFunc,
                                                                               tCancelDist,
                                                                               tFiffInfo,
                                                                               sensorTypeFiffConstant);

    //create new Tree Item
    if(!m_pInterpolationItem)
    {
        m_pInterpolationItem = new CshInterpolationItem(t3DEntityParent, Data3DTreeModelItemTypes::CshInterpolationItem, QStringLiteral("CshInterpolation"));
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
        data.setValue(dSmallSensorData); //@TODO this should use fSmallSensorData
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
        data.setValue(dSmallSensorData); //@TODO this should use fSmallSensorData
        this->setData(data, Data3DTreeModelItemRoles::RTData);

        //Add data to worker
        m_pSensorRtDataWorker->addData(dSmallSensorData.cast<float>());
    }
}


//*************************************************************************************************************

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


//*************************************************************************************************************

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


//*************************************************************************************************************

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


//*************************************************************************************************************

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


//*************************************************************************************************************

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


//*************************************************************************************************************

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


//*************************************************************************************************************

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


//*************************************************************************************************************

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
    //@TODO implement this
}


//*************************************************************************************************************

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

void CshSensorDataTreeItem::onNewRtData(const VectorXf &sensorData)
{
    //@TODO do this with a signal?
    if(m_pInterpolationItem)
    {
        m_pInterpolationItem->addNewRtData(sensorData);

    }
//    QVariant data;
//    data.setValue(sensorData);
//    emit rtCshSensorValuesChanged(data);
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
    //@TODO implement this
}


//*************************************************************************************************************

void CshSensorDataTreeItem::onInterpolationFunctionChanged(const QVariant &sInterpolationFunction)
{
    //@TODO implement this
}


//*************************************************************************************************************
