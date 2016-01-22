//=============================================================================================================
/**
* @file     brainrtdata.cpp
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
* @brief    BrainRTDataTreeItem class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "BrainRTDataTreeItem.h"


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

BrainRTDataTreeItem::BrainRTDataTreeItem(int iType, const QString &text)
: AbstractTreeItem(iType, text)
, m_bIsInit(false)
, m_pSourceLocRtDataWorker(new RtSourceLocDataWorker(this))
{
    connect(m_pSourceLocRtDataWorker, &RtSourceLocDataWorker::newRtData,
            this, &BrainRTDataTreeItem::onNewRtData);

    this->setEditable(false);
    this->setToolTip("Real time data");
}


//*************************************************************************************************************

BrainRTDataTreeItem::~BrainRTDataTreeItem()
{
    if(m_pSourceLocRtDataWorker->isRunning()) {
        m_pSourceLocRtDataWorker->stop();
        qDebug()<<"m_pSourceLocRtDataWorker stopped";
    }
}


//*************************************************************************************************************

QVariant BrainRTDataTreeItem::data(int role) const
{
    return AbstractTreeItem::data(role);
}


//*************************************************************************************************************

void  BrainRTDataTreeItem::setData(const QVariant& value, int role)
{
    AbstractTreeItem::setData(value, role);
}


//*************************************************************************************************************

bool BrainRTDataTreeItem::init(const MNEForwardSolution& tForwardSolution, const QByteArray& arraySurfaceVertColor, int iHemi, const VectorXi& vecLabelIds, const QList<FSLIB::Label>& lLabels)
{   
    //Set hemisphere information as item's data
    this->setData(iHemi, BrainRTDataTreeItemRoles::RTHemi);

    //Set data based on clusterd or full source space
    bool isClustered = tForwardSolution.src[iHemi].isClustered();

    if(isClustered) {
        //Source Space IS clustered
        switch(iHemi) {
            case 0:
                this->setData(0, BrainRTDataTreeItemRoles::RTStartIdx);
                this->setData(tForwardSolution.src[0].cluster_info.centroidSource_rr.size() - 1, BrainRTDataTreeItemRoles::RTEndIdx);
                break;

            case 1:
                this->setData(tForwardSolution.src[0].cluster_info.centroidSource_rr.size(), BrainRTDataTreeItemRoles::RTStartIdx);
                this->setData(tForwardSolution.src[0].cluster_info.centroidSource_rr.size() + tForwardSolution.src[1].cluster_info.centroidSource_rr.size() - 1, BrainRTDataTreeItemRoles::RTEndIdx);
                break;
        }
    } else {
        //Source Space is NOT clustered
        switch(iHemi) {
            case 0:
                this->setData(0, BrainRTDataTreeItemRoles::RTStartIdx);
                this->setData(tForwardSolution.src[0].nuse - 1, BrainRTDataTreeItemRoles::RTEndIdx);
                break;

            case 1:
                this->setData(tForwardSolution.src[0].nuse, BrainRTDataTreeItemRoles::RTStartIdx);
                this->setData(tForwardSolution.src[0].nuse + tForwardSolution.src[1].nuse - 1, BrainRTDataTreeItemRoles::RTEndIdx);
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

        this->setData(data, BrainRTDataTreeItemRoles::RTVertNo);
    }

    //Add surface meta information as item children
    BrainTreeMetaItem* pItemRTDataStreamStatus = new BrainTreeMetaItem(BrainTreeMetaItemTypes::RTDataStreamStatus, "Stream data on/off");
    connect(pItemRTDataStreamStatus, &BrainTreeMetaItem::checkStateChanged,
            this, &BrainRTDataTreeItem::onCheckStateWorkerChanged);
    *this<<pItemRTDataStreamStatus;
    pItemRTDataStreamStatus->setCheckable(true);
    pItemRTDataStreamStatus->setCheckState(Qt::Unchecked);
    data.setValue(false);
    pItemRTDataStreamStatus->setData(data, BrainTreeMetaItemRoles::RTDataStreamStatus);

    BrainTreeMetaItem* pItemVisuaizationType = new BrainTreeMetaItem(BrainTreeMetaItemTypes::RTDataVisualizationType, "Vertex based");
    connect(pItemVisuaizationType, &BrainTreeMetaItem::rtDataVisualizationTypeChanged,
            this, &BrainRTDataTreeItem::onVisualizationTypeChanged);
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
            this, &BrainRTDataTreeItem::onColormapTypeChanged);
    *this<<pItemColormapType;
    data.setValue(QString("Hot Negative 2"));
    pItemColormapType->setData(data, BrainTreeMetaItemRoles::RTDataColormapType);

    BrainTreeMetaItem* pItemSourceLocNormValue = new BrainTreeMetaItem(BrainTreeMetaItemTypes::RTDataNormalizationValue, "0.1");
    connect(pItemSourceLocNormValue, &BrainTreeMetaItem::rtDataNormalizationValueChanged,
            this, &BrainRTDataTreeItem::onDataNormalizationValueChanged);
    *this<<pItemSourceLocNormValue;
    data.setValue(0.1);
    pItemSourceLocNormValue->setData(data, BrainTreeMetaItemRoles::RTDataNormalizationValue);

    BrainTreeMetaItem *pItemStreamingInterval = new BrainTreeMetaItem(BrainTreeMetaItemTypes::RTDataTimeInterval, "1000");
    connect(pItemStreamingInterval, &BrainTreeMetaItem::rtDataTimeIntervalChanged,
            this, &BrainRTDataTreeItem::onTimeIntervalChanged);
    *this<<pItemStreamingInterval;
    data.setValue(1000);
    pItemStreamingInterval->setData(data, BrainTreeMetaItemRoles::RTDataTimeInterval);

    BrainTreeMetaItem *pItemLoopedStreaming = new BrainTreeMetaItem(BrainTreeMetaItemTypes::RTDataLoopedStreaming, "Looping on/off");
    connect(pItemLoopedStreaming, &BrainTreeMetaItem::checkStateChanged,
            this, &BrainRTDataTreeItem::onCheckStateLoopedStateChanged);
    pItemLoopedStreaming->setCheckable(true);
    pItemLoopedStreaming->setCheckState(Qt::Checked);
    *this<<pItemLoopedStreaming;

    BrainTreeMetaItem *pItemAveragedStreaming = new BrainTreeMetaItem(BrainTreeMetaItemTypes::RTDataNumberAverages, "1");
    connect(pItemAveragedStreaming, &BrainTreeMetaItem::rtDataNumberAveragesChanged,
            this, &BrainRTDataTreeItem::onNumberAveragesChanged);
    *this<<pItemAveragedStreaming;
    data.setValue(1);
    pItemAveragedStreaming->setData(data, BrainTreeMetaItemRoles::RTDataNumberAverages);

    //set rt data corresponding to the hemisphere
    m_pSourceLocRtDataWorker->setSurfaceData(arraySurfaceVertColor, this->data(BrainRTDataTreeItemRoles::RTVertNo).value<VectorXi>());
    m_pSourceLocRtDataWorker->setAnnotationData(vecLabelIds, lLabels);

    m_bIsInit = true;

    return true;
}


//*************************************************************************************************************

bool BrainRTDataTreeItem::addData(const MNESourceEstimate& tSourceEstimate)
{
    if(!m_bIsInit) {
        qDebug()<<"BrainRTDataTreeItem::updateData - Rt Item has not been initialized yet!";
        return false;
    }

    int iStartIdx = this->data(BrainRTDataTreeItemRoles::RTStartIdx).toInt();
    int iEndIdx = this->data(BrainRTDataTreeItemRoles::RTEndIdx).toInt();

    if(iStartIdx >= tSourceEstimate.data.rows() || iEndIdx >= tSourceEstimate.data.rows()) {
        qDebug()<<"BrainRTDataTreeItem::addData - Start and/or end index do not match with incoming data";
        return false;
    }

    MatrixXd subData = tSourceEstimate.data.block(iStartIdx, 0, iEndIdx-iStartIdx+1, tSourceEstimate.data.cols());

    m_pSourceLocRtDataWorker->addData(subData);

    return true;
}


//*************************************************************************************************************

void BrainRTDataTreeItem::onColorInfoOriginChanged(const QByteArray& arrayVertColor)
{
    m_pSourceLocRtDataWorker->setSurfaceData(arrayVertColor, this->data(BrainRTDataTreeItemRoles::RTVertNo).value<VectorXi>());
}


//*************************************************************************************************************

void BrainRTDataTreeItem::onCheckStateWorkerChanged(const Qt::CheckState& checkState)
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

void BrainRTDataTreeItem::onNewRtData(const QByteArray& sourceColorSamples)
{
    emit rtVertColorChanged(sourceColorSamples);
}


//*************************************************************************************************************

void BrainRTDataTreeItem::onColormapTypeChanged(const QString& sColormapType)
{
    m_pSourceLocRtDataWorker->setColormapType(sColormapType);
}


//*************************************************************************************************************

void BrainRTDataTreeItem::onTimeIntervalChanged(int iMSec)
{
    m_pSourceLocRtDataWorker->setInterval(iMSec);
}


//*************************************************************************************************************

void BrainRTDataTreeItem::onDataNormalizationValueChanged(double dValue)
{
    m_pSourceLocRtDataWorker->setNormalization(dValue);
}


//*************************************************************************************************************

void BrainRTDataTreeItem::onVisualizationTypeChanged(const QString& sVisType)
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

void BrainRTDataTreeItem::onCheckStateLoopedStateChanged(const Qt::CheckState& checkState)
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

void BrainRTDataTreeItem::onNumberAveragesChanged(int iNumAvr)
{
    m_pSourceLocRtDataWorker->setNumberAverages(iNumAvr);
}

