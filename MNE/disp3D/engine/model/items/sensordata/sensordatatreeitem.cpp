//=============================================================================================================
/**
* @file     sensordatatreeitem.cpp
* @author   Felix Griesau <felix.griesau@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     Month, Year
*
* @section  LICENSE
*
* Copyright (C) Year, Felix Griesau and Matti Hamalainen. All rights reserved.
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
* @brief    sensordatatreeitem class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sensordatatreeitem.h"
#include "../../workers/rtSourceLoc/rtsourcelocdataworker.h"
#include "../common/metatreeitem.h"

#include <mne/mne_sourceestimate.h>
#include <mne/mne_forwardsolution.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;
using namespace DISP3DLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SensorDataTreeItem::SensorDataTreeItem(int iType, const QString &text)
: AbstractTreeItem(iType,text)
, m_bIsDataInit(false)
{
    initItem();
}

//*************************************************************************************************************

SensorDataTreeItem::~SensorDataTreeItem()
{
    if(m_pSourceLocRtDataWorker->isRunning()) {
        m_pSourceLocRtDataWorker->stop();
        delete m_pSourceLocRtDataWorker;
    }
}


//*************************************************************************************************************

void SensorDataTreeItem::initItem()
{
    this->setEditable(false);
    this->setToolTip("MNE Sensordata item");

    //Add items
    QList<QStandardItem*> list;
    QVariant data;

    MetaTreeItem* pItemStreamStatus = new MetaTreeItem(MetaTreeItemTypes::StreamStatus, "Stream data on/off");
    connect(pItemStreamStatus, &MetaTreeItem::checkStateChanged,
            this, &SensorDataTreeItem::onCheckStateWorkerChanged);
    list << pItemStreamStatus;
    list << new QStandardItem(pItemStreamStatus->toolTip());
    this->appendRow(list);
    pItemStreamStatus->setCheckable(true);
    pItemStreamStatus->setCheckState(Qt::Unchecked);

    data.setValue(false);
    pItemStreamStatus->setData(data, MetaTreeItemRoles::StreamStatus);

    MetaTreeItem* pItemVisuaizationType = new MetaTreeItem(MetaTreeItemTypes::VisualizationType, "Vertex based");
    connect(pItemVisuaizationType, &MetaTreeItem::dataChanged,
            this, &SensorDataTreeItem::onVisualizationTypeChanged);
    list.clear();
    list << pItemVisuaizationType;
    list << new QStandardItem(pItemVisuaizationType->toolTip());
    this->appendRow(list);
    data.setValue(QString("Vertex based"));
    pItemVisuaizationType->setData(data, MetaTreeItemRoles::VisualizationType);

    MetaTreeItem* pItemColormapType = new MetaTreeItem(MetaTreeItemTypes::ColormapType, "Hot");
    connect(pItemColormapType, &MetaTreeItem::dataChanged,
            this, &SensorDataTreeItem::onColormapTypeChanged);
    list.clear();
    list << pItemColormapType;
    list << new QStandardItem(pItemColormapType->toolTip());
    this->appendRow(list);
    data.setValue(QString("Hot"));
    pItemColormapType->setData(data, MetaTreeItemRoles::ColormapType);

    MetaTreeItem* pItemSourceLocNormValue = new MetaTreeItem(MetaTreeItemTypes::DistributedSourceLocThreshold, "0.0,5.5,15");
    connect(pItemSourceLocNormValue, &MetaTreeItem::dataChanged,
            this, &SensorDataTreeItem::onDataNormalizationValueChanged);
    list.clear();
    list << pItemSourceLocNormValue;
    list << new QStandardItem(pItemSourceLocNormValue->toolTip());
    this->appendRow(list);
    data.setValue(QVector3D(0.0,5.5,15));
    pItemSourceLocNormValue->setData(data, MetaTreeItemRoles::DistributedSourceLocThreshold);

    MetaTreeItem *pItemStreamingInterval = new MetaTreeItem(MetaTreeItemTypes::StreamingTimeInterval, "50");
    connect(pItemStreamingInterval, &MetaTreeItem::dataChanged,
            this, &SensorDataTreeItem::onTimeIntervalChanged);
    list.clear();
    list << pItemStreamingInterval;
    list << new QStandardItem(pItemStreamingInterval->toolTip());
    this->appendRow(list);
    data.setValue(50);
    pItemStreamingInterval->setData(data, MetaTreeItemRoles::StreamingTimeInterval);

    MetaTreeItem *pItemLoopedStreaming = new MetaTreeItem(MetaTreeItemTypes::LoopedStreaming, "Looping on/off");
    connect(pItemLoopedStreaming, &MetaTreeItem::checkStateChanged,
            this, &SensorDataTreeItem::onCheckStateLoopedStateChanged);
    pItemLoopedStreaming->setCheckable(true);
    pItemLoopedStreaming->setCheckState(Qt::Checked);
    list.clear();
    list << pItemLoopedStreaming;
    list << new QStandardItem(pItemLoopedStreaming->toolTip());
    this->appendRow(list);

    MetaTreeItem *pItemAveragedStreaming = new MetaTreeItem(MetaTreeItemTypes::NumberAverages, "1");
    connect(pItemAveragedStreaming, &MetaTreeItem::dataChanged,
            this, &SensorDataTreeItem::onNumberAveragesChanged);
    list.clear();
    list << pItemAveragedStreaming;
    list << new QStandardItem(pItemAveragedStreaming->toolTip());
    this->appendRow(list);
    data.setValue(1);
    pItemAveragedStreaming->setData(data, MetaTreeItemRoles::NumberAverages);
}


//*************************************************************************************************************

void SensorDataTreeItem::init(  const MatrixX3f& matSurfaceVertColor,
                                const VectorXi& vecLabelIds,
                                const QList<FSLIB::Label>& lLabels)
{


        //Source Space IS clustered
        this->setData(0, Data3DTreeModelItemRoles::RTStartIdxLeftHemi);
        this->setData(matSurfaceVertColor.size() - 1, Data3DTreeModelItemRoles::RTEndIdxLeftHemi);

        this->setData(matSurfaceVertColor.size(), Data3DTreeModelItemRoles::RTStartIdxRightHemi);
        this->setData(matSurfaceVertColor.size() + matSurfaceVertColor.size() - 1, Data3DTreeModelItemRoles::RTEndIdxRightHemi);

    QVariant data;

    for(int i = 0; i < tForwardSolution.src.size(); ++i) {
        if(isClustered) {
            //When clustered source space, the idx no's are the annotation labels. Take the .cluster_info.centroidVertno instead.
            VectorXi clustVertNo(tForwardSolution.src[i].cluster_info.centroidVertno.size());
            for(int j = 0; j < clustVertNo.rows(); ++j) {
                clustVertNo(j) = tForwardSolution.src[i].cluster_info.centroidVertno.at(j);
            }
            data.setValue(clustVertNo);
        } else {
            data.setValue(tForwardSolution.src[i].vertno);
        }

        if(i == 0) {
            this->setData(data, Data3DTreeModelItemRoles::RTVertNoLeftHemi);
        } else if (i == 1) {
            this->setData(data, Data3DTreeModelItemRoles::RTVertNoRightHemi);
        }
    }

    //Add meta information as item children
    QList<QStandardItem*> list;

    QString sIsClustered = isClustered ? "Clustered" : "Full";
    MetaTreeItem* pItemSourceSpaceType = new MetaTreeItem(MetaTreeItemTypes::SourceSpaceType, sIsClustered);
    pItemSourceSpaceType->setEditable(false);
    list.clear();
    list << pItemSourceSpaceType;
    list << new QStandardItem(pItemSourceSpaceType->toolTip());
    this->appendRow(list);
    data.setValue(sIsClustered);
    pItemSourceSpaceType->setData(data, MetaTreeItemRoles::SourceSpaceType);

    //set rt data corresponding to the hemisphere
    if(!m_pSourceLocRtDataWorker) {
        m_pSourceLocRtDataWorker = new RtSourceLocDataWorker();
    }

    connect(m_pSourceLocRtDataWorker.data(), &RtSourceLocDataWorker::newRtData,
            this, &SensorDataTreeItem::onNewRtData);

    m_pSourceLocRtDataWorker->setSurfaceData(this->data(Data3DTreeModelItemRoles::RTVertNoLeftHemi).value<VectorXi>(),
                                             this->data(Data3DTreeModelItemRoles::RTVertNoRightHemi).value<VectorXi>(),
                                             tForwardSolution.src[0].neighbor_vert,
                                             tForwardSolution.src[1].neighbor_vert,
                                             tForwardSolution.src[0].rr,
                                             tForwardSolution.src[1].rr);

    m_pSourceLocRtDataWorker->setSurfaceColor(matSurfaceVertColorLeftHemi,
                                             matSurfaceVertColorRightHemi);

    m_pSourceLocRtDataWorker->setAnnotationData(vecLabelIdsLeftHemi,
                                                vecLabelIdsRightHemi,
                                                lLabelsLeftHemi,
                                                lLabelsRightHemi);

    m_bIsDataInit = true;
}


//*************************************************************************************************************

void SensorDataTreeItem::addData(const MatrixXd& tSensorData)
{
    if(!m_bIsDataInit) {
        qDebug() << "SensorDataTreeItem::addData - Rt source loc item has not been initialized yet!";
        return;
    }

    //Set new data into item's data. The set data is for eample needed in the delegate to calculate the histogram.
    QVariant data;
    data.setValue(tSensorData.data);
    this->setData(data, Data3DTreeModelItemRoles::RTData);

    if(m_pSourceLocRtDataWorker) {
        m_pSourceLocRtDataWorker->addData(tSensorData.data);
    }
}


//*************************************************************************************************************

void SensorDataTreeItem::setLoopState(bool state)
{
    QList<QStandardItem*> lItems = this->findChildren(MetaTreeItemTypes::LoopedStreaming);

    for(int i = 0; i < lItems.size(); i++) {
        if(MetaTreeItem* pAbstractItem = dynamic_cast<MetaTreeItem*>(lItems.at(i))) {
            pAbstractItem->setCheckState(state == true ? Qt::Checked : Qt::Unchecked);
            QVariant data;
            data.setValue(state);
            pAbstractItem->setData(data, MetaTreeItemRoles::LoopedStreaming);
        }
    }
}


//*************************************************************************************************************

void SensorDataTreeItem::setStreamingActive(bool state)
{
    QList<QStandardItem*> lItems = this->findChildren(MetaTreeItemTypes::StreamStatus);

    for(int i = 0; i < lItems.size(); i++) {
        if(MetaTreeItem* pAbstractItem = dynamic_cast<MetaTreeItem*>(lItems.at(i))) {
            pAbstractItem->setCheckState(state == true ? Qt::Checked : Qt::Unchecked);
            QVariant data;
            data.setValue(state);
            pAbstractItem->setData(data, MetaTreeItemRoles::StreamStatus);
        }
    }
}


//*************************************************************************************************************

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


//*************************************************************************************************************

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


//*************************************************************************************************************

void SensorDataTreeItem::setColortable(const QString& sColortable)
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

void SensorDataTreeItem::setVisualizationType(const QString& sVisualizationType)
{
    QList<QStandardItem*> lItems = this->findChildren(MetaTreeItemTypes::VisualizationType);

    for(int i = 0; i < lItems.size(); i++) {
        if(MetaTreeItem* pAbstractItem = dynamic_cast<MetaTreeItem*>(lItems.at(i))) {
            QVariant data;
            data.setValue(sVisualizationType);
            pAbstractItem->setData(data, MetaTreeItemRoles::VisualizationType);
            pAbstractItem->setData(data, Qt::DisplayRole);
        }
    }
}


//*************************************************************************************************************

void SensorDataTreeItem::setNormalization(const QVector3D& vecThresholds)
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

void SensorDataTreeItem::setColorOrigin(const MatrixX3f& matVertColorLeftHemisphere, const MatrixX3f& matVertColorRightHemisphere)
{
    m_pSourceLocRtDataWorker->setSurfaceColor(matVertColorLeftHemisphere,
                                             matVertColorRightHemisphere);
}


//*************************************************************************************************************

void SensorDataTreeItem::onCheckStateWorkerChanged(const Qt::CheckState& checkState)
{
    if(m_pSourceLocRtDataWorker) {
        if(checkState == Qt::Checked) {
            m_pSourceLocRtDataWorker->start();
        } else if(checkState == Qt::Unchecked) {
            m_pSourceLocRtDataWorker->stop();
        }
    }
}


//*************************************************************************************************************

void SensorDataTreeItem::onNewRtData(const QPair<MatrixX3f, MatrixX3f>& sourceColorSamples)
{
    emit rtVertColorChanged(sourceColorSamples);
}


//*************************************************************************************************************

void SensorDataTreeItem::onColormapTypeChanged(const QVariant& sColormapType)
{
    if(sColormapType.canConvert<QString>()) {
        if(m_pSourceLocRtDataWorker) {
            m_pSourceLocRtDataWorker->setColormapType(sColormapType.toString());
        }
    }
}


//*************************************************************************************************************

void SensorDataTreeItem::onTimeIntervalChanged(const QVariant& iMSec)
{
    if(iMSec.canConvert<int>()) {
        if(m_pSourceLocRtDataWorker) {
            m_pSourceLocRtDataWorker->setInterval(iMSec.toInt());
        }
    }
}


//*************************************************************************************************************

void SensorDataTreeItem::onDataNormalizationValueChanged(const QVariant& vecThresholds)
{
    if(vecThresholds.canConvert<QVector3D>()) {
        if(m_pSourceLocRtDataWorker) {
            m_pSourceLocRtDataWorker->setNormalization(vecThresholds.value<QVector3D>());
        }
    }
}


//*************************************************************************************************************

void SensorDataTreeItem::onVisualizationTypeChanged(const QVariant& sVisType)
{
    if(sVisType.canConvert<QString>()) {
        int iVisType = Data3DTreeModelItemRoles::VertexBased;

        if(sVisType.toString() == "Annotation based") {
            iVisType = Data3DTreeModelItemRoles::AnnotationBased;
        }

        if(sVisType.toString() == "Smoothing based") {
            iVisType = Data3DTreeModelItemRoles::SmoothingBased;
        }

        if(m_pSourceLocRtDataWorker) {
            m_pSourceLocRtDataWorker->setVisualizationType(iVisType);
        }
    }
}


//*************************************************************************************************************

void SensorDataTreeItem::onCheckStateLoopedStateChanged(const Qt::CheckState& checkState)
{
    if(m_pSourceLocRtDataWorker) {
        if(checkState == Qt::Checked) {
            m_pSourceLocRtDataWorker->setLoop(true);
        } else if(checkState == Qt::Unchecked) {
            m_pSourceLocRtDataWorker->setLoop(false);
        }
    }
}


//*************************************************************************************************************

void SensorDataTreeItem::onNumberAveragesChanged(const QVariant& iNumAvr)
{
    if(iNumAvr.canConvert<int>()) {
        if(m_pSourceLocRtDataWorker) {
            m_pSourceLocRtDataWorker->setNumberAverages(iNumAvr.toInt());
        }
    }
}

