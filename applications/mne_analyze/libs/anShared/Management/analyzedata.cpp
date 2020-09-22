//=============================================================================================================
/**
 * @file     analyzedata.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Christoph Dinh, Lorenz Esch, Lars Debor, Simon Heinke. All rights reserved.
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
, m_pData(new AnalyzeDataModel(this))
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
    if(QStandardItem* pItem = m_pData->itemFromIndex(index)) {
        QString sModelPath = pItem->toolTip();
        QFileInfo info (sModelPath);

        QMessageBox msgBox;
        msgBox.setText("Are you sure you want to remove "+info.fileName()+"?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        int ret = msgBox.exec();

        if(ret == QMessageBox::Yes) {
            // Check if the parent of the deleted item holds any other data. If not delete it as well.
            if(QStandardItem* pItemParent = m_pData->itemFromIndex(index.parent())) {
                if(pItemParent->rowCount() <= 1) {
                    if(m_pData->removeRows(index.parent().row(), 1)) {
                        if(m_pData->rowCount() == 0) {
                            emit modelIsEmpty();
                        }
                        qInfo() << "[AnalyzeData::removeModel] Removed model and parent at index" << index;
                        return true;
                    } else {
                        qInfo() << "[AnalyzeData::removeModel] Could not remove model and parent at index" << index;
                        return false;
                    }
                } else {
                    if(m_pData->removeRows(index.row(), 1, index.parent())) {
                        if(m_pData->rowCount() == 0) {
                            emit modelIsEmpty();
                        }
                        qInfo() << "[AnalyzeData::removeModel] Removed model at index" << index;
                        return true;
                    } else {
                        qInfo() << "[AnalyzeData::removeModel] Could not remove model at index" << index;
                        return false;
                    }
                }
            }
        }
    }

    return false;
}
