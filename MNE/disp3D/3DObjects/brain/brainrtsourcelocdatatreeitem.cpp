//=============================================================================================================
/**
* @file     brainrtsourcelocdatatreeitem.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    BrainRTSourceLocDataTreeItem class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainrtsourcelocdatatreeitem.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;
using namespace DISP3DLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainRTSourceLocDataTreeItem::BrainRTSourceLocDataTreeItem(int iType, const QString &text)
: AbstractTreeItem(iType, text)
, m_bIsInit(false)
, m_pSourceLocRtDataWorker(new RtSourceLocDataWorker(this))
{
    connect(m_pSourceLocRtDataWorker, &RtSourceLocDataWorker::newRtData,
            this, &BrainRTSourceLocDataTreeItem::onNewRtData);

    this->setEditable(false);
    this->setToolTip("Real time source localization data");
}


//*************************************************************************************************************

BrainRTSourceLocDataTreeItem::~BrainRTSourceLocDataTreeItem()
{
    if(m_pSourceLocRtDataWorker->isRunning()) {
        m_pSourceLocRtDataWorker->stop();
        qDebug()<<"m_pSourceLocRtDataWorker stopped";
    }
}


//*************************************************************************************************************

QVariant BrainRTSourceLocDataTreeItem::data(int role) const
{
    return AbstractTreeItem::data(role);
}


//*************************************************************************************************************

void BrainRTSourceLocDataTreeItem::setData(const QVariant& value, int role)
{
    AbstractTreeItem::setData(value, role);
}


//*************************************************************************************************************

bool BrainRTSourceLocDataTreeItem::init(const MNEForwardSolution& tForwardSolution, const QByteArray& arraySurfaceVertColor, int iHemi, const VectorXi& vecLabelIds, const QList<FSLIB::Label>& lLabels)
{   
    //Set hemisphere information as item's data
    this->setData(iHemi, BrainRTSourceLocDataTreeItemRoles::RTHemi);

    //Set data based on clusterd or full source space
    bool isClustered = tForwardSolution.src[iHemi].isClustered();

    if(isClustered) {
        //Source Space IS clustered
        switch(iHemi) {
            case 0:
                this->setData(0, BrainRTSourceLocDataTreeItemRoles::RTStartIdx);
                this->setData(tForwardSolution.src[0].cluster_info.centroidSource_rr.size() - 1, BrainRTSourceLocDataTreeItemRoles::RTEndIdx);
                break;

            case 1:
                this->setData(tForwardSolution.src[0].cluster_info.centroidSource_rr.size(), BrainRTSourceLocDataTreeItemRoles::RTStartIdx);
                this->setData(tForwardSolution.src[0].cluster_info.centroidSource_rr.size() + tForwardSolution.src[1].cluster_info.centroidSource_rr.size() - 1, BrainRTSourceLocDataTreeItemRoles::RTEndIdx);
                break;
        }
    } else {
        //Source Space is NOT clustered
        switch(iHemi) {
            case 0:
                this->setData(0, BrainRTSourceLocDataTreeItemRoles::RTStartIdx);
                this->setData(tForwardSolution.src[0].nuse - 1, BrainRTSourceLocDataTreeItemRoles::RTEndIdx);
                break;

            case 1:
                this->setData(tForwardSolution.src[0].nuse, BrainRTSourceLocDataTreeItemRoles::RTStartIdx);
                this->setData(tForwardSolution.src[0].nuse + tForwardSolution.src[1].nuse - 1, BrainRTSourceLocDataTreeItemRoles::RTEndIdx);
                break;
        }
    }

    QVariant data;

    if(iHemi != -1 && iHemi < tForwardSolution.src.size()) {
        if(isClustered) {
            //When clustered source space, the idx no's are the annotation labels. Take the .cluster_info.centroidVertno instead.
            VectorXi clustVertNo(tForwardSolution.src[iHemi].cluster_info.centroidVertno.size());
            for(int i = 0; i <clustVertNo.rows(); i++) {
                clustVertNo(i) = tForwardSolution.src[iHemi].cluster_info.centroidVertno.at(i);
            }
            data.setValue(clustVertNo);
        } else {
            data.setValue(tForwardSolution.src[iHemi].vertno);
        }

        this->setData(data, BrainRTSourceLocDataTreeItemRoles::RTVertNo);
    }

    //Add meta information as item children
    BrainTreeMetaItem* pItemRTDataStreamStatus = new BrainTreeMetaItem(BrainTreeMetaItemTypes::RTDataStreamStatus, "Stream data on/off");
    connect(pItemRTDataStreamStatus, &BrainTreeMetaItem::checkStateChanged,
            this, &BrainRTSourceLocDataTreeItem::onCheckStateWorkerChanged);
    *this<<pItemRTDataStreamStatus;
    pItemRTDataStreamStatus->setCheckable(true);
    pItemRTDataStreamStatus->setCheckState(Qt::Unchecked);
    data.setValue(false);
    pItemRTDataStreamStatus->setData(data, BrainTreeMetaItemRoles::RTDataStreamStatus);

    BrainTreeMetaItem* pItemVisuaizationType = new BrainTreeMetaItem(BrainTreeMetaItemTypes::RTDataVisualizationType, "Vertex based");
    connect(pItemVisuaizationType, &BrainTreeMetaItem::rtDataVisualizationTypeChanged,
            this, &BrainRTSourceLocDataTreeItem::onVisualizationTypeChanged);
    *this<<pItemVisuaizationType;
    data.setValue(QString("Single Vertex"));
    pItemVisuaizationType->setData(data, BrainTreeMetaItemRoles::RTDataVisualizationType);

    QString sIsClustered = isClustered ? "Clustered" : "Full";
    BrainTreeMetaItem* pItemSourceSpaceType = new BrainTreeMetaItem(BrainTreeMetaItemTypes::RTDataSourceSpaceType, sIsClustered);
    pItemSourceSpaceType->setEditable(false);
    *this<<pItemSourceSpaceType;
    data.setValue(sIsClustered);
    pItemSourceSpaceType->setData(data, BrainTreeMetaItemRoles::RTDataSourceSpaceType);

    BrainTreeMetaItem* pItemColormapType = new BrainTreeMetaItem(BrainTreeMetaItemTypes::RTDataColormapType, "Hot Negative 2");
    connect(pItemColormapType, &BrainTreeMetaItem::rtDataColormapTypeChanged,
            this, &BrainRTSourceLocDataTreeItem::onColormapTypeChanged);
    *this<<pItemColormapType;
    data.setValue(QString("Hot Negative 2"));
    pItemColormapType->setData(data, BrainTreeMetaItemRoles::RTDataColormapType);

    BrainTreeMetaItem* pItemSourceLocNormValue = new BrainTreeMetaItem(BrainTreeMetaItemTypes::RTDataNormalizationValue, "10.0");
    connect(pItemSourceLocNormValue, &BrainTreeMetaItem::rtDataNormalizationValueChanged,
            this, &BrainRTSourceLocDataTreeItem::onDataNormalizationValueChanged);
    *this<<pItemSourceLocNormValue;
    data.setValue(10.0);
    pItemSourceLocNormValue->setData(data, BrainTreeMetaItemRoles::RTDataNormalizationValue);

    BrainTreeMetaItem *pItemStreamingInterval = new BrainTreeMetaItem(BrainTreeMetaItemTypes::RTDataTimeInterval, "1000");
    connect(pItemStreamingInterval, &BrainTreeMetaItem::rtDataTimeIntervalChanged,
            this, &BrainRTSourceLocDataTreeItem::onTimeIntervalChanged);
    *this<<pItemStreamingInterval;
    data.setValue(1000);
    pItemStreamingInterval->setData(data, BrainTreeMetaItemRoles::RTDataTimeInterval);

    BrainTreeMetaItem *pItemLoopedStreaming = new BrainTreeMetaItem(BrainTreeMetaItemTypes::RTDataLoopedStreaming, "Looping on/off");
    connect(pItemLoopedStreaming, &BrainTreeMetaItem::checkStateChanged,
            this, &BrainRTSourceLocDataTreeItem::onCheckStateLoopedStateChanged);
    pItemLoopedStreaming->setCheckable(true);
    pItemLoopedStreaming->setCheckState(Qt::Checked);
    *this<<pItemLoopedStreaming;

    BrainTreeMetaItem *pItemAveragedStreaming = new BrainTreeMetaItem(BrainTreeMetaItemTypes::RTDataNumberAverages, "1");
    connect(pItemAveragedStreaming, &BrainTreeMetaItem::rtDataNumberAveragesChanged,
            this, &BrainRTSourceLocDataTreeItem::onNumberAveragesChanged);
    *this<<pItemAveragedStreaming;
    data.setValue(1);
    pItemAveragedStreaming->setData(data, BrainTreeMetaItemRoles::RTDataNumberAverages);

    //set rt data corresponding to the hemisphere
    m_pSourceLocRtDataWorker->setSurfaceData(arraySurfaceVertColor, this->data(BrainRTSourceLocDataTreeItemRoles::RTVertNo).value<VectorXi>());
    m_pSourceLocRtDataWorker->setAnnotationData(vecLabelIds, lLabels);

    m_bIsInit = true;

    return true;
}


//*************************************************************************************************************

bool BrainRTSourceLocDataTreeItem::addData(const MNESourceEstimate& tSourceEstimate)
{
    if(!m_bIsInit) {
        qDebug()<<"BrainRTSourceLocDataTreeItem::updateData - Rt Item has not been initialized yet!";
        return false;
    }

    int iStartIdx = this->data(BrainRTSourceLocDataTreeItemRoles::RTStartIdx).toInt();
    int iEndIdx = this->data(BrainRTSourceLocDataTreeItemRoles::RTEndIdx).toInt();

//    qDebug()<<"BrainRTSourceLocDataTreeItem::addData - iStartIdx"<<iStartIdx;
//    qDebug()<<"BrainRTSourceLocDataTreeItem::addData - iEndIdx"<<iEndIdx;
//    qDebug()<<"BrainRTSourceLocDataTreeItem::addData - tSourceEstimate.data.rows()"<<tSourceEstimate.data.rows();

    if(iStartIdx >= tSourceEstimate.data.rows() || iEndIdx >= tSourceEstimate.data.rows()) {
        qDebug()<<"BrainRTSourceLocDataTreeItem::addData - Start and/or end index do not match with incoming data";
        return false;
    }

    MatrixXd subData = tSourceEstimate.data.block(iStartIdx, 0, iEndIdx-iStartIdx+1, tSourceEstimate.data.cols());

    m_pSourceLocRtDataWorker->addData(subData);

    return true;
}


//*************************************************************************************************************

void BrainRTSourceLocDataTreeItem::setLoopState(bool state)
{
    QList<QStandardItem*> lItems = this->findChildren(BrainTreeMetaItemTypes::RTDataLoopedStreaming);

    for(int i = 0; i < lItems.size(); i++) {
        if(BrainTreeMetaItem* pAbstractItem = dynamic_cast<BrainTreeMetaItem*>(lItems.at(i))) {
            pAbstractItem->setCheckState(state == true ? Qt::Checked : Qt::Unchecked);
            QVariant data;
            data.setValue(state);
            pAbstractItem->setData(data, BrainTreeMetaItemRoles::RTDataLoopedStreaming);
        }
    }
}


//*************************************************************************************************************

void BrainRTSourceLocDataTreeItem::setStreamingActive(bool state)
{
    QList<QStandardItem*> lItems = this->findChildren(BrainTreeMetaItemTypes::RTDataStreamStatus);

    for(int i = 0; i < lItems.size(); i++) {
        if(BrainTreeMetaItem* pAbstractItem = dynamic_cast<BrainTreeMetaItem*>(lItems.at(i))) {
            pAbstractItem->setCheckState(state == true ? Qt::Checked : Qt::Unchecked);
            QVariant data;
            data.setValue(state);
            pAbstractItem->setData(data, BrainTreeMetaItemRoles::RTDataStreamStatus);
        }
    }
}


//*************************************************************************************************************

void BrainRTSourceLocDataTreeItem::setTimeInterval(int iMSec)
{
    QList<QStandardItem*> lItems = this->findChildren(BrainTreeMetaItemTypes::RTDataTimeInterval);

    for(int i = 0; i < lItems.size(); i++) {
        if(BrainTreeMetaItem* pAbstractItem = dynamic_cast<BrainTreeMetaItem*>(lItems.at(i))) {
            qDebug()<<"BrainRTSourceLocDataTreeItem::setTimeInterval";
            QVariant data;
            data.setValue(iMSec);
            pAbstractItem->setData(data, BrainTreeMetaItemRoles::RTDataTimeInterval);
            pAbstractItem->setData(data, Qt::DisplayRole);
        }
    }
}


//*************************************************************************************************************

void BrainRTSourceLocDataTreeItem::setNumberAverages(int iNumberAverages)
{
    QList<QStandardItem*> lItems = this->findChildren(BrainTreeMetaItemTypes::RTDataNumberAverages);

    for(int i = 0; i < lItems.size(); i++) {
        if(BrainTreeMetaItem* pAbstractItem = dynamic_cast<BrainTreeMetaItem*>(lItems.at(i))) {
            QVariant data;
            data.setValue(iNumberAverages);
            pAbstractItem->setData(data, BrainTreeMetaItemRoles::RTDataNumberAverages);
            pAbstractItem->setData(data, Qt::DisplayRole);
        }
    }
}


//*************************************************************************************************************

void BrainRTSourceLocDataTreeItem::setColortable(const QString& sColortable)
{
    QList<QStandardItem*> lItems = this->findChildren(BrainTreeMetaItemTypes::RTDataColormapType);

    for(int i = 0; i < lItems.size(); i++) {
        if(BrainTreeMetaItem* pAbstractItem = dynamic_cast<BrainTreeMetaItem*>(lItems.at(i))) {
            QVariant data;
            data.setValue(sColortable);
            pAbstractItem->setData(data, BrainTreeMetaItemRoles::RTDataColormapType);
            pAbstractItem->setData(data, Qt::DisplayRole);
        }
    }
}


//*************************************************************************************************************

void BrainRTSourceLocDataTreeItem::setVisualizationType(const QString& sVisualizationType)
{
    QList<QStandardItem*> lItems = this->findChildren(BrainTreeMetaItemTypes::RTDataVisualizationType);

    for(int i = 0; i < lItems.size(); i++) {
        if(BrainTreeMetaItem* pAbstractItem = dynamic_cast<BrainTreeMetaItem*>(lItems.at(i))) {
            QVariant data;
            data.setValue(sVisualizationType);
            pAbstractItem->setData(data, BrainTreeMetaItemRoles::RTDataVisualizationType);
            pAbstractItem->setData(data, Qt::DisplayRole);
        }
    }
}


//*************************************************************************************************************

void BrainRTSourceLocDataTreeItem::setNormalization(double dNormalization)
{
    QList<QStandardItem*> lItems = this->findChildren(BrainTreeMetaItemTypes::RTDataNormalizationValue);

    for(int i = 0; i < lItems.size(); i++) {
        if(BrainTreeMetaItem* pAbstractItem = dynamic_cast<BrainTreeMetaItem*>(lItems.at(i))) {
            QVariant data;
            data.setValue(dNormalization);
            pAbstractItem->setData(data, BrainTreeMetaItemRoles::RTDataNormalizationValue);
            pAbstractItem->setData(data, Qt::DisplayRole);
        }
    }
}


//*************************************************************************************************************

void BrainRTSourceLocDataTreeItem::onColorInfoOriginChanged(const QByteArray& arrayVertColor)
{
    m_pSourceLocRtDataWorker->setSurfaceData(arrayVertColor, this->data(BrainRTSourceLocDataTreeItemRoles::RTVertNo).value<VectorXi>());
}


//*************************************************************************************************************

void BrainRTSourceLocDataTreeItem::onCheckStateWorkerChanged(const Qt::CheckState& checkState)
{
    if(checkState == Qt::Checked) {
        qDebug()<<"Start stc worker";
        m_pSourceLocRtDataWorker->start();
    } else if(checkState == Qt::Unchecked) {
        qDebug()<<"Stop stc worker";
        m_pSourceLocRtDataWorker->stop();
    }    
}


//*************************************************************************************************************

void BrainRTSourceLocDataTreeItem::onNewRtData(const QByteArray& sourceColorSamples)
{
    emit rtVertColorChanged(sourceColorSamples);
}


//*************************************************************************************************************

void BrainRTSourceLocDataTreeItem::onColormapTypeChanged(const QString& sColormapType)
{
    m_pSourceLocRtDataWorker->setColormapType(sColormapType);
}


//*************************************************************************************************************

void BrainRTSourceLocDataTreeItem::onTimeIntervalChanged(int iMSec)
{
    m_pSourceLocRtDataWorker->setInterval(iMSec);


}


//*************************************************************************************************************

void BrainRTSourceLocDataTreeItem::onDataNormalizationValueChanged(double dValue)
{
    m_pSourceLocRtDataWorker->setNormalization(dValue);
}


//*************************************************************************************************************

void BrainRTSourceLocDataTreeItem::onVisualizationTypeChanged(const QString& sVisType)
{
    int iVisType = BrainRTDataVisualizationTypes::VertexBased;

    if(sVisType == "Annotation based") {
        iVisType = BrainRTDataVisualizationTypes::AnnotationBased;
    }

    if(sVisType == "Smoothing based") {
        iVisType = BrainRTDataVisualizationTypes::SmoothingBased;
    }

    m_pSourceLocRtDataWorker->setVisualizationType(iVisType);
}


//*************************************************************************************************************

void BrainRTSourceLocDataTreeItem::onCheckStateLoopedStateChanged(const Qt::CheckState& checkState)
{
    if(checkState == Qt::Checked) {
        qDebug()<<"Looped streaming active";
        m_pSourceLocRtDataWorker->setLoop(true);
    } else if(checkState == Qt::Unchecked) {
        qDebug()<<"Looped streaming inactive";
        m_pSourceLocRtDataWorker->setLoop(false);
    }
}


//*************************************************************************************************************

void BrainRTSourceLocDataTreeItem::onNumberAveragesChanged(int iNumAvr)
{
    m_pSourceLocRtDataWorker->setNumberAverages(iNumAvr);
}

