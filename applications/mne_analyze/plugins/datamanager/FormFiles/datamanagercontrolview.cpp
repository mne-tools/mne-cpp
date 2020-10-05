//=============================================================================================================
/**
 * @file     datamanagerview.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     August, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch, Lars Debor, Simon Heinke. All rights reserved.
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
 * @brief    Contains the declaration of the DataManagerView class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "datamanagercontrolview.h"
#include "ui_datamanagerview.h"

#include <anShared/Model/analyzedatamodel.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QTreeView>
#include <QStandardItemModel>
#include <QDebug>
#include <QMenu>
#include <QKeyEvent>

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DataManagerControlView::DataManagerControlView(QWidget *parent)
: QWidget(parent)
, m_pUi(new Ui::DataManagerView)
{
    m_pUi->setupUi(this);
    m_pUi->m_pTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_pUi->m_pTreeView, &QTreeView::customContextMenuRequested,
            this, &DataManagerControlView::customMenuRequested, Qt::UniqueConnection);
}

//=============================================================================================================

DataManagerControlView::~DataManagerControlView()
{
    delete m_pUi;
}

//=============================================================================================================

void DataManagerControlView::setModel(QAbstractItemModel *pModel)
{
    m_pUi->m_pTreeView->setModel(pModel);

    connect(m_pUi->m_pTreeView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &DataManagerControlView::onCurrentItemChanged, Qt::UniqueConnection);
//    connect(static_cast<ANSHAREDLIB::AnalyzeDataModel*>(pModel), &ANSHAREDLIB::AnalyzeDataModel::newFileAdded,
//            this, &DataManagerControlView::onNewFileLoaded);
    connect(static_cast<ANSHAREDLIB::AnalyzeDataModel*>(pModel), &ANSHAREDLIB::AnalyzeDataModel::newItemIndex,
            this, &DataManagerControlView::onNewItemIndex);
}

//=============================================================================================================

void DataManagerControlView::customMenuRequested(QPoint pos)
{
    QString sToolTip = m_pUi->m_pTreeView->model()->data(m_pUi->m_pTreeView->indexAt(pos), Qt::ToolTipRole).toString();

//    if(sToolTip != "Subject item") {
//        QMenu *menu = new QMenu(this);

//        QAction* pAction = new QAction("Remove", this);
//        connect(pAction, &QAction::triggered, [=]() {
//            emit removeItem(m_pUi->m_pTreeView->indexAt(pos));
//        });

//        menu->addAction(pAction);
//        menu->popup(m_pUi->m_pTreeView->viewport()->mapToGlobal(pos));
//    }

    ANSHAREDLIB::AnalyzeDataModel *pModel = qobject_cast<ANSHAREDLIB::AnalyzeDataModel *>(m_pUi->m_pTreeView->model());
    QStandardItem* pItem = pModel->itemFromIndex(m_pUi->m_pTreeView->indexAt(pos));

    switch (pItem->data(ITEM_TYPE).value<int>()){
        case SUBJECT: {
            QMenu *menu = new QMenu(this);

            QAction* pRemoveAction = new QAction("Remove Subject", this);
            connect(pRemoveAction, &QAction::triggered, [=]() {
                emit removeItem(m_pUi->m_pTreeView->indexAt(pos));
            });

            menu->addAction(pRemoveAction);
            menu->popup(m_pUi->m_pTreeView->viewport()->mapToGlobal(pos));
            break;
        }
        case SESSION: {
            QMenu *menu = new QMenu(this);

            QAction* pRemoveAction = new QAction("Remove Session", this);
            connect(pRemoveAction, &QAction::triggered, [=]() {
                emit removeItem(m_pUi->m_pTreeView->indexAt(pos));
            });

            QMenu* pMoveMenu = new QMenu("Move Session to ...");

            menu->addMenu(pMoveMenu);
            menu->addAction(pRemoveAction);
            menu->popup(m_pUi->m_pTreeView->viewport()->mapToGlobal(pos));
            break;
        }
        case DATA: {
            QMenu *menu = new QMenu(this);

            QAction* pRemoveAction = new QAction("Remove Data", this);
            connect(pRemoveAction, &QAction::triggered, [=]() {
                emit removeItem(m_pUi->m_pTreeView->indexAt(pos));
            });

            QMenu* pMoveMenu = new QMenu("Move Data to ...");

            menu->addMenu(pMoveMenu);
            menu->addAction(pRemoveAction);
            menu->popup(m_pUi->m_pTreeView->viewport()->mapToGlobal(pos));
            break;
        }
        default:{
            qDebug() << "DataManagerControlView::customMenuRequested - default";
        }
    }

//    for(QStandardItem* item : pModel->takeColumn(0)){
//        qDebug() << item;
//    }
}

//=============================================================================================================

void DataManagerControlView::onCurrentItemChanged(const QItemSelection &selected,
                                                  const QItemSelection &deselected)
{
    Q_UNUSED(deselected)
    if(selected.indexes().empty()) {
        return;
    }

    emit selectedItemChanged(selected.indexes().first());

    if(QStandardItemModel *pModel = qobject_cast<QStandardItemModel *>(m_pUi->m_pTreeView->model())) {        
        if(QStandardItem* pItem = pModel->itemFromIndex(selected.indexes().first())) {
            if(!pItem->data().isNull()) {
                emit selectedModelChanged(pItem->data());
            }
        }
    }
}

//=============================================================================================================

void DataManagerControlView::onNewFileLoaded(int iSubject,
                                             int iModel)
{
    m_pUi->m_pTreeView->selectionModel()->select(m_pUi->m_pTreeView->model()->index(iModel, 0, m_pUi->m_pTreeView->model()->index(iSubject, 0)),
                                                 QItemSelectionModel::ClearAndSelect);
}

//=============================================================================================================

void DataManagerControlView::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key_Delete:
            if(m_pUi->m_pTreeView->model()->data(m_pUi->m_pTreeView->currentIndex(), Qt::ToolTipRole).toString() != "Subject item") {
                emit removeItem(m_pUi->m_pTreeView->currentIndex());
            }
            break;

        default:
            QWidget::keyPressEvent(event);
    }
}

//=============================================================================================================

void DataManagerControlView::onNewItemIndex(QModelIndex itemIndex)
{
    m_pUi->m_pTreeView->selectionModel()->select(itemIndex, QItemSelectionModel::ClearAndSelect);
    m_pUi->m_pTreeView->expandAll();
}
