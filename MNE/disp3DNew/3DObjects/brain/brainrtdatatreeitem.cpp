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

//    BrainTreeItem *itemStreamingSpeed = new BrainTreeItem(BrainTreeModelItemTypes::RTDataStreamingSpeed, "Streaming speed");
//    *this<<itemStreamingSpeed;

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

    m_pStcDataWorker->addData(tSourceEstimate.data);

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

void BrainRTDataTreeItem::onStcSample(const VectorXd& sample)
{
    int iStartIdx = this->data(BrainRTDataTreeItemRoles::RTStartIdx).toInt();
    int iEndIdx = this->data(BrainRTDataTreeItemRoles::RTEndIdx).toInt();

    qDebug()<<"BrainRTDataTreeItem::onStcSample - sample.rows(): "<<sample.rows();
    qDebug()<<"BrainRTDataTreeItem::onStcSample - iStartIdx"<<iStartIdx;
    qDebug()<<"BrainRTDataTreeItem::onStcSample - iEndIdx"<<iEndIdx;

    //Calculate/Transform acutal colors from rtSorceLoc samples
    VectorXd subSamples = sample.segment(iStartIdx, iEndIdx-iStartIdx+1);
    MatrixX3f matVertColors(subSamples.rows(), 3);
    QString sColorMapType = m_pItemColormapType->data(BrainTreeItemRoles::RTDataColormapType).toString();

    if(sColorMapType == "Hot Negative 1") {
        for(int i = 0; i<subSamples.rows(); i++) {
            qint32 iVal = subSamples(i) > 255 ? 255 : subSamples(i) < 0 ? 0 : subSamples(i);

            QRgb qRgb;
            qRgb = ColorMap::valueToHotNegative1((float)iVal/255.0);

            QColor colSample(qRgb);
            matVertColors(i, 0) = colSample.redF();
            matVertColors(i, 1) = colSample.greenF();
            matVertColors(i, 2) = colSample.blueF();
        }
    }

    if(sColorMapType == "Hot Negative 2") {
        for(int i = 0; i<subSamples.rows(); i++) {
            qint32 iVal = subSamples(i) > 255 ? 255 : subSamples(i) < 0 ? 0 : subSamples(i);

            QRgb qRgb;
            qRgb = ColorMap::valueToHotNegative2((float)iVal/255.0);

            QColor colSample(qRgb);
            matVertColors(i, 0) = colSample.redF();
            matVertColors(i, 1) = colSample.greenF();
            matVertColors(i, 2) = colSample.blueF();
        }
    }

    if(sColorMapType == "Hot") {
        for(int i = 0; i<subSamples.rows(); i++) {
            qint32 iVal = subSamples(i) > 255 ? 255 : subSamples(i) < 0 ? 0 : subSamples(i);

            QRgb qRgb;
            qRgb = ColorMap::valueToHot((float)iVal/255.0);

            QColor colSample(qRgb);
            matVertColors(i, 0) = colSample.redF();
            matVertColors(i, 1) = colSample.greenF();
            matVertColors(i, 2) = colSample.blueF();
        }
    }

    emit rtDataUpdated(matVertColors, this->data(BrainRTDataTreeItemRoles::RTVerticesIdx).value<VectorXi>());
}

