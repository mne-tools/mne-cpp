//=============================================================================================================
/**
 * @file     sensordatatreeitem.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     June, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lars Debor, Lorenz Esch. All rights reserved.
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
 * @brief    SensorDataTreeItem class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sensordatatreeitem.h"
#include "../common/metatreeitem.h"
#include "../../workers/rtSensorData/rtsensordatacontroller.h"
#include "../common/gpuinterpolationitem.h"
#include "../common/abstractmeshtreeitem.h"
#include "../../3dhelpers/custommesh.h"
#include "../../materials/pervertexphongalphamaterial.h"

#include <mne/mne_bem.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector3D>
#include <QGeometryRenderer>
#include <Qt3DCore/QTransform>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace DISP3DLIB;
using namespace MNELIB;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SensorDataTreeItem::SensorDataTreeItem(int iType,
                                       const QString &text,
                                       bool bUseGPU)
: AbstractTreeItem(iType,text)
, m_bIsDataInit(false)
, m_bUseGPU(bUseGPU)
, m_pSensorRtDataWorkController(new RtSensorDataController())
{
    initItem();
}

//=============================================================================================================

SensorDataTreeItem::~SensorDataTreeItem()
{
    m_pSensorRtDataWorkController->deleteLater();
}

//=============================================================================================================

void SensorDataTreeItem::initData(const MNEBemSurface &bemSurface,
                                  const FiffInfo &fiffInfo,
                                  const QString &sSensorType,
                                  Qt3DCore::QEntity* p3DEntityParent)
{
    if(m_bIsDataInit == true) {
        qDebug("SensorDataTreeItem::initData - Item is already initialized");
    }

    this->setData(0, Data3DTreeModelItemRoles::Data);

    // map passed sensor type string to fiff constant
    fiff_int_t sensorTypeFiffConstant;
    if (sSensorType.toStdString() == "MEG") {
        sensorTypeFiffConstant = FIFFV_MEG_CH;
    } else if (sSensorType.toStdString() == "EEG") {
        sensorTypeFiffConstant = FIFFV_EEG_CH;
    } else {
        qDebug() << "SensorDataTreeItem::initData - unknown sensor type. Returning ...";
        return;
    }

    //Fill QVector with the right sensor positions, exclude bad channels
    //Only take EEG with V as unit or MEG magnetometers with T as unit
    QVector<Vector3f> vecSensorPos;
    m_iUsedSensors.clear();
    int iCounter = 0;
    for(const FiffChInfo &info : fiffInfo.chs) {
        if(info.kind == sensorTypeFiffConstant &&
                (info.unit == FIFF_UNIT_T || info.unit == FIFF_UNIT_V)) {
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

    //Create InterpolationItems for CPU or GPU usage
    if(m_bUseGPU) {
        if(!m_pInterpolationItemGPU) {
            m_pInterpolationItemGPU = new GpuInterpolationItem(p3DEntityParent,
                                                               Data3DTreeModelItemTypes::GpuInterpolationItem,
                                                               QStringLiteral("3D Plot"));

            m_pInterpolationItemGPU->initData(bemSurface.rr,
                                              bemSurface.nn,
                                              bemSurface.tris);

            QList<QStandardItem*> list;
            list << m_pInterpolationItemGPU;
            list << new QStandardItem(m_pInterpolationItemGPU->toolTip());
            this->appendRow(list);
        }

        m_pSensorRtDataWorkController->setStreamSmoothedData(false);

        connect(m_pSensorRtDataWorkController.data(), &RtSensorDataController::newInterpolationMatrixAvailable,
                        this, &SensorDataTreeItem::onNewInterpolationMatrixAvailable);

        connect(m_pSensorRtDataWorkController.data(), &RtSensorDataController::newRtRawDataAvailable,
                this, &SensorDataTreeItem::onNewRtRawDataAvailable);
    } else {
        if(!m_pInterpolationItemCPU) {
            m_pInterpolationItemCPU = new AbstractMeshTreeItem(p3DEntityParent,
                                                            Data3DTreeModelItemTypes::AbstractMeshItem,
                                                            QStringLiteral("3D Plot"));

            //Create color from curvature information with default gyri and sulcus colors
            MatrixX4f matVertColor = AbstractMeshTreeItem::createVertColor(bemSurface.rr.rows());

            m_pInterpolationItemCPU->setMeshData(bemSurface.rr,
                                                 bemSurface.nn,
                                                 bemSurface.tris,
                                                 matVertColor,
                                                 Qt3DRender::QGeometryRenderer::Triangles);

            QList<QStandardItem*> list;
            list << m_pInterpolationItemCPU;
            list << new QStandardItem(m_pInterpolationItemCPU->toolTip());
            this->appendRow(list);

            //Set material to enable sorting
            QPointer<PerVertexPhongAlphaMaterial> pBemMaterial = new PerVertexPhongAlphaMaterial(true);
            m_pInterpolationItemCPU->setMaterial(pBemMaterial);
        }

        connect(m_pSensorRtDataWorkController.data(), &RtSensorDataController::newRtSmoothedDataAvailable,
                this, &SensorDataTreeItem::onNewRtSmoothedDataAvailable);
    }

    //Setup worker
    m_pSensorRtDataWorkController->setInterpolationInfo(bemSurface.rr,
                                                        bemSurface.neighbor_vert,
                                                        vecSensorPos,
                                                        fiffInfo,
                                                        sensorTypeFiffConstant);

    //Init complete
    m_bIsDataInit = true;
}

//=============================================================================================================

void SensorDataTreeItem::addData(const MatrixXd &tSensorData)
{
    if(!m_bIsDataInit) {
        qDebug() << "SensorDataTreeItem::addData - item has not been initialized yet!";
        return;
    }

    //if more data then needed is provided
    const int iSensorSize = m_iUsedSensors.size();
    if(tSensorData.rows() > iSensorSize)
    {
        MatrixXd dSmallSensorData(iSensorSize, tSensorData.cols());
        for(int i = 0 ; i < iSensorSize; ++i)
        {
            //Set bad channels to zero so that the data does not corrupt while the bad channels vertex is weighted by surrounding sensors
            if(m_iSensorsBad.contains(m_iUsedSensors[i])) {
                dSmallSensorData.row(i).setZero();
            } else {
                dSmallSensorData.row(i) = tSensorData.row(m_iUsedSensors[i]);
            }
        }
        //Set new data into item's data.
        QVariant data;
        data.setValue(dSmallSensorData);
        this->setData(data, Data3DTreeModelItemRoles::Data);

        //Add data to worker
        m_pSensorRtDataWorkController->addData(dSmallSensorData);
    }
    else
    {
        //Set bad channels to zero
        MatrixXd dSmallSensorData = tSensorData;
        for(int i = 0 ; i < dSmallSensorData.rows(); ++i)
        {
            if(m_iSensorsBad.contains(m_iUsedSensors[i])) {
                dSmallSensorData.row(i).setZero();
            }
        }

        //Set new data into item's data.
        QVariant data;
        data.setValue(dSmallSensorData);
        this->setData(data, Data3DTreeModelItemRoles::Data);

        //Add data to worker
        m_pSensorRtDataWorkController->addData(dSmallSensorData);
    }
}

//=============================================================================================================

void SensorDataTreeItem::setLoopState(bool bState)
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

//=============================================================================================================

void SensorDataTreeItem::setStreamingState(bool bState)
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

//=============================================================================================================

void SensorDataTreeItem::setTimeInterval(int iMSec)
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

//=============================================================================================================

void SensorDataTreeItem::setNumberAverages(int iNumberAverages)
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

//=============================================================================================================

void SensorDataTreeItem::setColormapType(const QString& sColormap)
{
    QList<QStandardItem*> lItems = this->findChildren(MetaTreeItemTypes::ColormapType);

    for(int i = 0; i < lItems.size(); i++) {
        if(MetaTreeItem* pAbstractItem = dynamic_cast<MetaTreeItem*>(lItems.at(i))) {
            QVariant data;
            data.setValue(sColormap);
            pAbstractItem->setData(data, MetaTreeItemRoles::ColormapType);
            pAbstractItem->setData(data, Qt::DisplayRole);
        }
    }
}

//=============================================================================================================

void SensorDataTreeItem::setThresholds(const QVector3D& vecThresholds)
{
    QList<QStandardItem*> lItems = this->findChildren(MetaTreeItemTypes::DataThreshold);

    for(int i = 0; i < lItems.size(); i++) {
        if(MetaTreeItem* pAbstractItem = dynamic_cast<MetaTreeItem*>(lItems.at(i))) {
            QVariant data;
            data.setValue(vecThresholds);
            pAbstractItem->setData(data, MetaTreeItemRoles::DataThreshold);

            QString sTemp = QString("%1,%2,%3").arg(vecThresholds.x()).arg(vecThresholds.y()).arg(vecThresholds.z());
            data.setValue(sTemp);
            pAbstractItem->setData(data, Qt::DisplayRole);
        }
    }
}

//=============================================================================================================

void SensorDataTreeItem::setCancelDistance(double dCancelDist)
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

//=============================================================================================================

void SensorDataTreeItem::setInterpolationFunction(const QString &sInterpolationFunction)
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

//=============================================================================================================

void SensorDataTreeItem::setSFreq(const double dSFreq)
{
    if(m_pSensorRtDataWorkController) {
        m_pSensorRtDataWorkController->setSFreq(dSFreq);
    }
}

//=============================================================================================================

void SensorDataTreeItem::setBadChannels(const FIFFLIB::FiffInfo &info)
{
    if(m_pSensorRtDataWorkController) {
        //Create bad channel idx list
        m_iSensorsBad.clear();
        for(const QString &bad : info.bads) {
            m_iSensorsBad.push_back(info.ch_names.indexOf(bad));
        }

        //qDebug() << "SensorDataTreeItem::setBadChannels - m_iSensorsBad" << m_iSensorsBad;
        m_pSensorRtDataWorkController->setBadChannels(info);
    }
}

//=============================================================================================================

void SensorDataTreeItem::setTransform(const Qt3DCore::QTransform& transform)
{
    if(m_pInterpolationItemGPU) {
        m_pInterpolationItemGPU->setTransform(transform);
    }

    if(m_pInterpolationItemCPU) {
        m_pInterpolationItemCPU->setTransform(transform);
    }
}

//=============================================================================================================

void SensorDataTreeItem::setTransform(const FiffCoordTrans& transform, bool bApplyInverse)
{
    if(m_pInterpolationItemGPU) {
        m_pInterpolationItemGPU->setTransform(transform, bApplyInverse);
    }

    if(m_pInterpolationItemCPU) {
        m_pInterpolationItemCPU->setTransform(transform, bApplyInverse);
    }
}

//=============================================================================================================

void SensorDataTreeItem::applyTransform(const Qt3DCore::QTransform& transform)
{
    if(m_pInterpolationItemGPU) {
        m_pInterpolationItemGPU->applyTransform(transform);
    }

    if(m_pInterpolationItemCPU) {
        m_pInterpolationItemCPU->applyTransform(transform);
    }
}

//=============================================================================================================

void SensorDataTreeItem::applyTransform(const FiffCoordTrans& transform, bool bApplyInverse)
{
    if(m_pInterpolationItemGPU) {
        m_pInterpolationItemGPU->applyTransform(transform, bApplyInverse);
    }

    if(m_pInterpolationItemCPU) {
        m_pInterpolationItemCPU->applyTransform(transform, bApplyInverse);
    }
}

//=============================================================================================================

void SensorDataTreeItem::initItem()
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip("Sensor Data item");

    //Add items
    QList<QStandardItem*> list;
    QVariant data;

    MetaTreeItem* pItemStreamStatus = new MetaTreeItem(MetaTreeItemTypes::StreamStatus, "Stream data on/off");
    list << pItemStreamStatus;
    list << new QStandardItem(pItemStreamStatus->toolTip());
    this->appendRow(list);
    pItemStreamStatus->setCheckable(true);
    pItemStreamStatus->setEditable(false);
    pItemStreamStatus->setCheckState(Qt::Unchecked);
    connect(pItemStreamStatus, &MetaTreeItem::checkStateChanged,
            this, &SensorDataTreeItem::onStreamingStateChanged);

    data.setValue(false);
    pItemStreamStatus->setData(data, MetaTreeItemRoles::StreamStatus);

    MetaTreeItem* pItemColormapType = new MetaTreeItem(MetaTreeItemTypes::ColormapType, "Hot");
    list.clear();
    list << pItemColormapType;
    list << new QStandardItem(pItemColormapType->toolTip());
    this->appendRow(list);
    data.setValue(QString("Hot"));
    pItemColormapType->setData(data, MetaTreeItemRoles::ColormapType);
    connect(pItemColormapType, &MetaTreeItem::dataChanged,
            this, &SensorDataTreeItem::onColormapTypeChanged);

    MetaTreeItem* pItemThreshold = new MetaTreeItem(MetaTreeItemTypes::DataThreshold, "0.0, 0.5,10.0");
    list.clear();
    list << pItemThreshold;
    list << new QStandardItem(pItemThreshold->toolTip());
    this->appendRow(list);
    data.setValue(QVector3D(0.0,5.5,15));
    pItemThreshold->setData(data, MetaTreeItemRoles::DataThreshold);
    connect(pItemThreshold, &MetaTreeItem::dataChanged,
            this, &SensorDataTreeItem::onDataThresholdChanged);

    MetaTreeItem *pItemStreamingInterval = new MetaTreeItem(MetaTreeItemTypes::StreamingTimeInterval, "17");
    list.clear();
    list << pItemStreamingInterval;
    list << new QStandardItem(pItemStreamingInterval->toolTip());
    this->appendRow(list);
    data.setValue(17);
    pItemStreamingInterval->setData(data, MetaTreeItemRoles::StreamingTimeInterval);
    connect(pItemStreamingInterval, &MetaTreeItem::dataChanged,
            this, &SensorDataTreeItem::onTimeIntervalChanged);

    MetaTreeItem *pItemLoopedStreaming = new MetaTreeItem(MetaTreeItemTypes::LoopedStreaming, "Looping on/off");
    pItemLoopedStreaming->setCheckable(true);
    pItemLoopedStreaming->setCheckState(Qt::Checked);
    list.clear();
    list << pItemLoopedStreaming;
    list << new QStandardItem(pItemLoopedStreaming->toolTip());
    this->appendRow(list);
    connect(pItemLoopedStreaming, &MetaTreeItem::checkStateChanged,
            this, &SensorDataTreeItem::onLoopStateChanged);

    MetaTreeItem *pItemAveragedStreaming = new MetaTreeItem(MetaTreeItemTypes::NumberAverages, "17");
    list.clear();
    list << pItemAveragedStreaming;
    list << new QStandardItem(pItemAveragedStreaming->toolTip());
    this->appendRow(list);
    data.setValue(1);
    pItemAveragedStreaming->setData(data, MetaTreeItemRoles::NumberAverages);
    connect(pItemAveragedStreaming, &MetaTreeItem::dataChanged,
            this, &SensorDataTreeItem::onNumberAveragesChanged);

    MetaTreeItem *pItemCancelDistance = new MetaTreeItem(MetaTreeItemTypes::CancelDistance, "0.05");
    list.clear();
    list << pItemCancelDistance;
    list << new QStandardItem(pItemCancelDistance->toolTip());
    this->appendRow(list);
    data.setValue(0.05);
    pItemCancelDistance->setData(data, MetaTreeItemRoles::CancelDistance);
    connect(pItemCancelDistance, &MetaTreeItem::dataChanged,
            this, &SensorDataTreeItem::onCancelDistanceChanged);

    MetaTreeItem* pInterpolationFunction = new MetaTreeItem(MetaTreeItemTypes::InterpolationFunction, "Cubic");
    list.clear();
    list << pInterpolationFunction;
    list << new QStandardItem(pInterpolationFunction->toolTip());
    this->appendRow(list);
    data.setValue(QString("Cubic"));
    pInterpolationFunction->setData(data, MetaTreeItemRoles::InterpolationFunction);
    connect(pInterpolationFunction, &MetaTreeItem::dataChanged,
            this, &SensorDataTreeItem::onInterpolationFunctionChanged);
}

//=============================================================================================================

void SensorDataTreeItem::onNewInterpolationMatrixAvailable(QSharedPointer<SparseMatrix<float> > pMatInterpolationMatrixLeftHemi)
{
    if(m_pInterpolationItemGPU)
    {
        m_pInterpolationItemGPU->setInterpolationMatrix(pMatInterpolationMatrixLeftHemi);
    }
}

//=============================================================================================================

void SensorDataTreeItem::onNewRtRawDataAvailable(const VectorXd &vecDataVector)
{
    if(m_pInterpolationItemGPU)
    {
        m_pInterpolationItemGPU->addNewRtData(vecDataVector.cast<float>());
    }
}

//=============================================================================================================

void SensorDataTreeItem::onNewRtSmoothedDataAvailable(const MatrixX4f &matColorMatrix)
{
    if(m_pInterpolationItemCPU)
    {
        m_pInterpolationItemCPU->setVertColor(matColorMatrix);
    }
}

//=============================================================================================================

void SensorDataTreeItem::onStreamingStateChanged(const Qt::CheckState &checkState)
{
    if(m_pSensorRtDataWorkController) {
        if(checkState == Qt::Checked) {
            m_pSensorRtDataWorkController->setStreamingState(true);
        } else if(checkState == Qt::Unchecked) {
            m_pSensorRtDataWorkController->setStreamingState(false);
        }
    }
}

//=============================================================================================================

void SensorDataTreeItem::onColormapTypeChanged(const QVariant &sColormapType)
{
    if(sColormapType.canConvert<QString>()) {
        if(m_bUseGPU) {
            if(m_pInterpolationItemGPU) {
                m_pInterpolationItemGPU->setColormapType(sColormapType.toString());
            }
        } else {
            if(m_pSensorRtDataWorkController) {
                m_pSensorRtDataWorkController->setColormapType(sColormapType.toString());
            }
        }
    }
}

//=============================================================================================================

void SensorDataTreeItem::onTimeIntervalChanged(const QVariant &iMSec)
{
    if(iMSec.canConvert<int>()) {
        if(m_pSensorRtDataWorkController) {
            m_pSensorRtDataWorkController->setTimeInterval(iMSec.toInt());
        }
    }
}

//=============================================================================================================

void SensorDataTreeItem::onDataThresholdChanged(const QVariant &vecThresholds)
{
    if(vecThresholds.canConvert<QVector3D>()) {
        if(m_bUseGPU) {
            if(m_pInterpolationItemGPU) {
                m_pInterpolationItemGPU->setThresholds(vecThresholds.value<QVector3D>());
            }
        } else {
            if(m_pSensorRtDataWorkController) {
                m_pSensorRtDataWorkController->setThresholds(vecThresholds.value<QVector3D>());
            }
        }
    }
}

//=============================================================================================================

void SensorDataTreeItem::onLoopStateChanged(const Qt::CheckState &checkState)
{
    if(m_pSensorRtDataWorkController) {
        if(checkState == Qt::Checked) {
           m_pSensorRtDataWorkController->setLoopState(true);
        } else if(checkState == Qt::Unchecked) {
           m_pSensorRtDataWorkController->setLoopState(false);
        }
    }
}

//=============================================================================================================

void SensorDataTreeItem::onNumberAveragesChanged(const QVariant &iNumAvr)
{
    if(iNumAvr.canConvert<int>()) {
        if(m_pSensorRtDataWorkController) {
           m_pSensorRtDataWorkController->setNumberAverages(iNumAvr.toInt());
        }
    }
}

//=============================================================================================================

void SensorDataTreeItem::onCancelDistanceChanged(const QVariant &dCancelDist)
{
    if(dCancelDist.canConvert<double>())
    {
        if(m_pSensorRtDataWorkController) {
            m_pSensorRtDataWorkController->setCancelDistance(dCancelDist.toDouble());
        }
    }
}

//=============================================================================================================

void SensorDataTreeItem::onInterpolationFunctionChanged(const QVariant &sInterpolationFunction)
{
    if(sInterpolationFunction.canConvert<QString>())
    {
        if(m_pSensorRtDataWorkController) {
            m_pSensorRtDataWorkController->setInterpolationFunction(sInterpolationFunction.toString());
        }
    }
}
