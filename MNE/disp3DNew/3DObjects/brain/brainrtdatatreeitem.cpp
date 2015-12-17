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
, m_sHemi("Unknown")
, m_pItemRTDataStreamStatus(new BrainTreeItem())
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
    m_sHemi = hemi;
    int iHemi = m_sHemi == "lh" ? 0 : 1;

    // Add data which is held by this BrainRTDataTreeItem
    QVariant data;

    data.setValue(tSourceEstimate.data);
    this->setData(data, BrainRTDataTreeItemRoles::RTData);

    data.setValue(tSourceEstimate.vertices);
    this->setData(data, BrainRTDataTreeItemRoles::RTVertices);

    data.setValue(tSourceEstimate.times);
    this->setData(data, BrainRTDataTreeItemRoles::RTTimes);

    //Add surface meta information as item children
    m_pItemRTDataStreamStatus = new BrainTreeItem(BrainTreeModelItemTypes::RTDataStreamStatus, "Stream data on/off");
    connect(m_pItemRTDataStreamStatus, &BrainTreeItem::checkStateChanged,
            this, &BrainRTDataTreeItem::onCheckStateChanged);
    *this<<m_pItemRTDataStreamStatus;
    m_pItemRTDataStreamStatus->setCheckable(true);
    m_pItemRTDataStreamStatus->setCheckState(Qt::Unchecked);
    data.setValue(false);
    m_pItemRTDataStreamStatus->setData(data, BrainTreeItemRoles::RTDataStreamStatus);

    QString sIsClustered = tForwardSolution.src[iHemi].isClustered() ? "Clustered source space" : "Full source space";
    BrainTreeItem *itemSourceSpaceType = new BrainTreeItem(BrainTreeModelItemTypes::RTDataSourceSpaceType, sIsClustered);
    *this<<itemSourceSpaceType;
    data.setValue(sIsClustered);
    itemSourceSpaceType->setData(data, BrainTreeItemRoles::RTDataSourceSpaceType);

    BrainTreeItem *itemColormapType = new BrainTreeItem(BrainTreeModelItemTypes::RTDataColormapType, "Hot Negative 2");
    *this<<itemColormapType;
    data.setValue(QString("Hot Negative 2"));
    itemColormapType->setData(data, BrainTreeItemRoles::RTDataColormapType);

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
        //TODO: Start stc worker
        qDebug()<<"Start stc worker";
        m_pStcDataWorker->start();
    } else if(checkState == Qt::Unchecked) {
        //TODO: Stop stc worker
        qDebug()<<"Stop stc worker";
        m_pStcDataWorker->stop();
    }
}


//*************************************************************************************************************

void BrainRTDataTreeItem::onStcSample(const VectorXd& sample)
{
    qDebug()<<"data received: "<<sample.rows();
    emit rtDataChanged(sample, this->data(BrainRTDataTreeItemRoles::RTVertices).value<VectorXi>());
}

