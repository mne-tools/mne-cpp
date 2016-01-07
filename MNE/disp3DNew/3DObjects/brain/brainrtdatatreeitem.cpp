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

using namespace DISP3DNEWLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainRTDataTreeItem::BrainRTDataTreeItem(const int &iType, const QString &text)
: AbstractTreeItem(iType, text)
, m_bInit(false)
, m_pRtDataWorker(new StcDataWorker(this))
{
    connect(m_pRtDataWorker, &StcDataWorker::stcSample,
            this, &BrainRTDataTreeItem::onStcSample);
}


//*************************************************************************************************************

BrainRTDataTreeItem::~BrainRTDataTreeItem()
{
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

bool BrainRTDataTreeItem::addData(const MNESourceEstimate& tSourceEstimate, const MNEForwardSolution& tForwardSolution, const QString& hemi)
{   
    //Find out which hemisphere we are working with and set as item's data
    int iHemi = hemi == "Left" ? 0 : hemi == "Right" ? 1 : -1;
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

    int iStartIdx = this->data(BrainRTDataTreeItemRoles::RTStartIdx).toInt();
    int iEndIdx = this->data(BrainRTDataTreeItemRoles::RTEndIdx).toInt();
    QVariant data;

    MatrixXd matHemisphereData = tSourceEstimate.data.block(iStartIdx, 0, iEndIdx-iStartIdx+1, tSourceEstimate.data.cols());
    data.setValue(matHemisphereData);
    qDebug()<<matHemisphereData.rows()<<"x"<<matHemisphereData.cols();
    this->setData(data, BrainRTDataTreeItemRoles::RTData);

    if(iHemi != -1 && iHemi < tForwardSolution.src.size()) {
        //Only take the vertno's corresponding to the hemisphere
        //VectorXi vecHemispehreVertNo = tForwardSolution.src[iHemi].vertno.segment(iStartIdx, iEndIdx-iStartIdx+1);
        data.setValue(tForwardSolution.src[iHemi].vertno); //TODO: When clustered source space, these idx no's are the annotation labels
        this->setData(data, BrainRTDataTreeItemRoles::RTVerticesIdx);
    }

    //Add surface meta information as item children
    BrainTreeItem* pItemRTDataStreamStatus = new BrainTreeItem(BrainTreeModelItemTypes::RTDataStreamStatus, "Stream data on/off");
    connect(pItemRTDataStreamStatus, &BrainTreeItem::checkStateChanged,
            this, &BrainRTDataTreeItem::onCheckStateChanged);
    *this<<pItemRTDataStreamStatus;
    pItemRTDataStreamStatus->setCheckable(true);
    pItemRTDataStreamStatus->setCheckState(Qt::Unchecked);
    data.setValue(false);
    pItemRTDataStreamStatus->setData(data, BrainTreeItemRoles::RTDataStreamStatus);

    QString sIsClustered = isClustered ? "Clustered" : "Full";
    BrainTreeItem* pItemSourceSpaceType = new BrainTreeItem(BrainTreeModelItemTypes::RTDataSourceSpaceType, sIsClustered);
    *this<<pItemSourceSpaceType;
    data.setValue(sIsClustered);
    pItemSourceSpaceType->setData(data, BrainTreeItemRoles::RTDataSourceSpaceType);

    BrainTreeItem* pItemColormapType = new BrainTreeItem(BrainTreeModelItemTypes::RTDataColormapType, "Hot Negative 2");
    connect(pItemColormapType, &BrainTreeItem::rtDataColormapTypeUpdated,
            this, &BrainRTDataTreeItem::onColormapTypeChanged);
    *this<<pItemColormapType;
    data.setValue(QString("Hot Negative 2"));
    pItemColormapType->setData(data, BrainTreeItemRoles::RTDataColormapType);

    BrainTreeItem* pItemSourceLocNormValue = new BrainTreeItem(BrainTreeModelItemTypes::RTDataNormalizationValue, "0.1");
    connect(pItemSourceLocNormValue, &BrainTreeItem::rtDataNormalizationValueUpdated,
            this, &BrainRTDataTreeItem::onDataNormalizationValueChanged);
    *this<<pItemSourceLocNormValue;
    data.setValue(0.1);
    pItemSourceLocNormValue->setData(data, BrainTreeItemRoles::RTDataNormalizationValue);

    BrainTreeItem *pItemStreamingInterval = new BrainTreeItem(BrainTreeModelItemTypes::RTDataTimeInterval, "1000");
    connect(pItemStreamingInterval, &BrainTreeItem::rtDataTimeIntervalUpdated,
            this, &BrainRTDataTreeItem::onTimeIntervalChanged);
    *this<<pItemStreamingInterval;
    data.setValue(1000);
    pItemStreamingInterval->setData(data, BrainTreeItemRoles::RTDataTimeInterval);

//    BrainTreeItem *itemLoopedStreaming = new BrainTreeItem(BrainTreeModelItemTypes::RTDataLoopedStreaming, "Looping on/off");
//    itemLoopedStreaming->setCheckable(true);
//    itemLoopedStreaming->setCheckState(Qt::Unchecked);
//    *this<<itemLoopedStreaming;

//    BrainTreeItem *itemAveragedStreaming = new BrainTreeItem(BrainTreeModelItemTypes::RTDataNumberAverages, "Number of Averages");
//    *this<<itemAveragedStreaming;

    //set rt data corresponding to the hemisphere
    m_pRtDataWorker->addData(matHemisphereData);

    m_bInit = true;

    return true;
}


//*************************************************************************************************************

bool BrainRTDataTreeItem::updateData(const MNESourceEstimate& tSourceEstimate)
{
    if(!m_bInit) {
        qDebug()<<"BrainRTDataTreeItem::updateData - Item was not initialized/filled with data yet!";
        return false;
    }

    int iStartIdx = this->data(BrainRTDataTreeItemRoles::RTStartIdx).toInt();
    int iEndIdx = this->data(BrainRTDataTreeItemRoles::RTEndIdx).toInt();
    m_pRtDataWorker->addData(tSourceEstimate.data.block(iStartIdx, 0, iEndIdx-iStartIdx+1, 3));

    QVariant data;
    MatrixXd subData = tSourceEstimate.data.block(iStartIdx, 0, iEndIdx-iStartIdx+1, 3);
    data.setValue(subData);
    this->setData(data, BrainRTDataTreeItemRoles::RTData);

    return true;
}


//*************************************************************************************************************

void BrainRTDataTreeItem::onCheckStateWorkerChanged(const Qt::CheckState& checkState)
{
    if(checkState == Qt::Checked) {
        qDebug()<<"Start stc worker";
        m_pRtDataWorker->start();
    } else if(checkState == Qt::Unchecked) {
        qDebug()<<"Stop stc worker";
        m_pRtDataWorker->stop();
    }
}


//*************************************************************************************************************

void BrainRTDataTreeItem::onStcSample(QByteArray sourceColorSamples)
{
    emit rtDataUpdated(sourceColorSamples, this->data(BrainRTDataTreeItemRoles::RTVerticesIdx).value<VectorXi>());
}


//*************************************************************************************************************

void BrainRTDataTreeItem::onColormapTypeChanged(const QString& sColormapType)
{
    m_pRtDataWorker->setColormapType(sColormapType);
}


//*************************************************************************************************************

void BrainRTDataTreeItem::onTimeIntervalChanged(const int& iMSec)
{
    m_pRtDataWorker->setInterval(iMSec);
}


//*************************************************************************************************************

void BrainRTDataTreeItem::onDataNormalizationValueChanged(const double& dValue)
{
    m_pRtDataWorker->setNormalization(dValue);
}
