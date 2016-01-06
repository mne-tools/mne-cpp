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
, m_pStcDataWorker(new StcDataWorker(this))
, m_dStcNormMax(10.0)
{
    connect(m_pStcDataWorker, &StcDataWorker::stcSample,
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
    if(tForwardSolution.src[iHemi].isClustered()) {
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

    data.setValue(tSourceEstimate.data);
    this->setData(data, BrainRTDataTreeItemRoles::RTData);

    if(iHemi != -1 && iHemi < tForwardSolution.src.size()) {
        data.setValue(tForwardSolution.src[iHemi].vertno); // TODO: When clustered source space, these idx no's are the annotation labels
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

    QString sIsClustered = tForwardSolution.src[iHemi].isClustered() ? "Clustered" : "Full";
    BrainTreeItem* pItemSourceSpaceType = new BrainTreeItem(BrainTreeModelItemTypes::RTDataSourceSpaceType, sIsClustered);
    *this<<pItemSourceSpaceType;
    data.setValue(sIsClustered);
    pItemSourceSpaceType->setData(data, BrainTreeItemRoles::RTDataSourceSpaceType);

    m_pItemColormapType = new BrainTreeItem(BrainTreeModelItemTypes::RTDataColormapType, "Hot Negative 2");
    *this<<m_pItemColormapType;
    data.setValue(QString("Hot Negative 2"));
    m_pItemColormapType->setData(data, BrainTreeItemRoles::RTDataColormapType);

    m_pItemSourceLocNormValue = new BrainTreeItem(BrainTreeModelItemTypes::RTDataNormalizationValue, "0.1");
    *this<<m_pItemSourceLocNormValue;
    data.setValue(0.1);
    m_pItemSourceLocNormValue->setData(data, BrainTreeItemRoles::RTDataNormalizationValue);

    BrainTreeItem *itemStreamingInterval = new BrainTreeItem(BrainTreeModelItemTypes::RTDataTimeInterval, "1000");
    connect(itemStreamingInterval, &BrainTreeItem::rtDataTimeIntervalUpdated,
            this, &BrainRTDataTreeItem::onStreamingIntervalChanged);
    *this<<itemStreamingInterval;
    data.setValue(1000);
    itemStreamingInterval->setData(data, BrainTreeItemRoles::RTDataTimeInterval);

//    BrainTreeItem *itemLoopedStreaming = new BrainTreeItem(BrainTreeModelItemTypes::RTDataLoopedStreaming, "Looping on/off");
//    itemLoopedStreaming->setCheckable(true);
//    itemLoopedStreaming->setCheckState(Qt::Unchecked);
//    *this<<itemLoopedStreaming;

//    BrainTreeItem *itemAveragedStreaming = new BrainTreeItem(BrainTreeModelItemTypes::RTDataNumberAverages, "Number of Averages");
//    *this<<itemAveragedStreaming;

    m_pStcDataWorker->addData(tSourceEstimate.data);

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

    m_pStcDataWorker->addData(tSourceEstimate.data, m_pItemColormapType->data(BrainTreeItemRoles::RTDataColormapType).toString());

    QVariant data;
    data.setValue(tSourceEstimate.data);
    this->setData(data, BrainRTDataTreeItemRoles::RTData);

    return true;
}


//*************************************************************************************************************

void BrainRTDataTreeItem::onCheckStateChanged(const Qt::CheckState& checkState)
{
    if(checkState == Qt::Checked) {
        qDebug()<<"Start stc worker";
        m_pStcDataWorker->start();
    } else if(checkState == Qt::Unchecked) {
        qDebug()<<"Stop stc worker";
        m_pStcDataWorker->stop();
    }
}


//*************************************************************************************************************

void BrainRTDataTreeItem::onStcSample(MatrixX3f sourceColorSamples)
{
    QTime time;
    time.start();

    int iStartIdx = this->data(BrainRTDataTreeItemRoles::RTStartIdx).toInt();
    int iEndIdx = this->data(BrainRTDataTreeItemRoles::RTEndIdx).toInt();

//    //Normalize source loc result and cut out the hemisphere part
//    Matrix3Xf subColorSamples = sourceSamples.block(iStartIdx, 0, iEndIdx-iStartIdx+1, 3);
//    subColorSamples /= (m_dStcNormMax/100.0) * m_pItemSourceLocNormValue->data(BrainTreeItemRoles::RTDataNormalizationValue).toDouble();

//    QString sColorMapType = m_pItemColormapType->data(BrainTreeItemRoles::RTDataColormapType).toString();
////    qDebug()<<"BrainRTDataTreeItem::onStcSample"<<time.elapsed()<<"msecs";

    emit rtDataUpdated(sourceColorSamples.block(iStartIdx, 0, iEndIdx-iStartIdx+1, 3), this->data(BrainRTDataTreeItemRoles::RTVerticesIdx).value<VectorXi>());

    }


//*************************************************************************************************************

void BrainRTDataTreeItem::onStreamingIntervalChanged(const int& usec)
{
    m_pStcDataWorker->setInterval(usec);
}
