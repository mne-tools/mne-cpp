//=============================================================================================================
/**
 * @file     analyzedata.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Christoph Dinh, Lorenz Esch, Lars Debor, Simon Heinke, Gabriel Motta. All rights reserved.
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
 * @brief    Definition of the Analyze Data Container Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "analyzedata.h"
#include <anShared/Management/communicator.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QVector>
#include <QSharedPointer>
#include <QString>
#include <QtConcurrent/QtConcurrent>
#include <QMessageBox>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AnalyzeData::AnalyzeData(QObject *pParent)
: QObject(pParent)
, m_pData(new DISPLIB::BidsViewModel(this))
{

}

//=============================================================================================================

AnalyzeData::~AnalyzeData()
{
}

//=============================================================================================================

QVector<QSharedPointer<AbstractModel> > AnalyzeData::getAllModels(QModelIndex parent) const
{
    QVector<QSharedPointer<AbstractModel> > lItems;

    for(int r = 0; r < m_pData->rowCount(parent); ++r) {
        QModelIndex index = m_pData->index(r, 0, parent);
        if(QSharedPointer<AbstractModel> pModel = m_pData->data(index).value<QSharedPointer<AbstractModel> >()) {
            lItems.append(pModel);
        }

        if( m_pData->hasChildren(index) ) {
            lItems.append(getAllModels(index));
            return lItems;
        }
    }
}

//=============================================================================================================

QVector<QSharedPointer<AbstractModel> > AnalyzeData::getModelsByType(MODEL_TYPE mtype,
                                                                     QModelIndex parent) const
{
    QVector<QSharedPointer<AbstractModel> > lItems;

    for(int r = 0; r < m_pData->rowCount(parent); ++r) {
        QModelIndex index = m_pData->index(r, 0, parent);
        if(QSharedPointer<AbstractModel> pModel = m_pData->data(index).value<QSharedPointer<AbstractModel> >()) {
            if (pModel->getType() == mtype) {
                lItems.append(pModel);
            }
        }

        if( m_pData->hasChildren(index) ) {
            lItems.append(getModelsByType(mtype, index));
            return lItems;
        }
    }
}

//=============================================================================================================

QSharedPointer<AbstractModel> AnalyzeData::getModelByName(const QString &sName) const
{
    QList<QStandardItem*> lItems = m_pData->findItems(sName, Qt::MatchRecursive);

    if(!lItems.isEmpty()) {
        return lItems.first()->data().value<QSharedPointer<AbstractModel> >();
    } else {
        return Q_NULLPTR;
    }
}

//=============================================================================================================

QSharedPointer<AbstractModel> AnalyzeData::getModelByPath(const QString& sPath,
                                                          QModelIndex parent) const
{
    for(int r = 0; r < m_pData->rowCount(parent); ++r) {
        QModelIndex index = m_pData->index(r, 0, parent);
        if(QStandardItem* pItem = m_pData->itemFromIndex(index)) {
            if (pItem->toolTip() == sPath) {
                if(QSharedPointer<AbstractModel> pModel = pItem->data().value<QSharedPointer<AbstractModel> >()) {
                    return pModel;
                }
            }
        }

        if(m_pData->hasChildren(index)) {
            return getModelByPath(sPath, index);
        }
    }

    return Q_NULLPTR;
}

//=============================================================================================================

QStandardItemModel* AnalyzeData::getDataModel()
{
    return m_pData;
}

//=============================================================================================================

bool AnalyzeData::removeModel(const QModelIndex& index)
{
    QStandardItem* pItem = m_pData->itemFromIndex(index);
    switch(pItem->data(BIDS_ITEM_TYPE).value<int>()){
    case BIDS_FUNCTIONALDATA:
    case BIDS_ANATOMICALDATA:
    case BIDS_BEHAVIORALDATA:
    case BIDS_AVERAGE:
    case BIDS_ANNOTATION:
        m_pCommu->publishEvent(EVENT_TYPE::MODEL_REMOVED, QVariant::fromValue(pItem->data().value<QSharedPointer<AbstractModel>>()));
        break;
    default:
        //No model associated with item
        break;
    }

    return m_pData->removeItem(index);
}

//=============================================================================================================

QStandardItem* AnalyzeData::addSubject(const QString &sSubjectName)
{
    return m_pData->itemFromIndex(m_pData->addSubject(sSubjectName));
}

//=============================================================================================================

void AnalyzeData::newSelection(const QModelIndex &index)
{
    switch(m_pData->itemFromIndex(index)->data(BIDS_ITEM_TYPE).value<int>()){
        case BIDS_UNKNOWN:
        case BIDS_FUNCTIONALDATA:
            m_SelectedFunctionalData = index;
            m_SelectedItem = index;
            break;
        default:
            m_SelectedItem = index;
            break;
    }
}

