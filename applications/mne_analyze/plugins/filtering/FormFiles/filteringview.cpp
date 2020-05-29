//=============================================================================================================
/**
 * @file     filteringview.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.2
 * @date     May, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the FilteringView class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filteringview.h"
#include "ui_filteringview.h"
#include <anShared/Management/analyzedatamodel.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QTreeView>
#include <QStandardItemModel>
#include <QDebug>
#include <QMenu>

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FilteringView::FilteringView(QWidget *parent)
: QWidget(parent)
, m_pUi(new Ui::FilteringView)
{
    m_pUi->setupUi(this);
    m_pUi->m_pTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_pUi->m_pTreeView, &QTreeView::customContextMenuRequested,
            this, &FilteringView::customMenuRequested, Qt::UniqueConnection);
}

//=============================================================================================================

FilteringView::~FilteringView()
{
    delete m_pUi;
}

//=============================================================================================================

void FilteringView::setModel(QAbstractItemModel *pModel)
{
    m_pUi->m_pTreeView->setModel(pModel);

    connect(m_pUi->m_pTreeView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &FilteringView::onCurrentItemChanged, Qt::UniqueConnection);
    //static_cast<ANSHAREDLIB::AnalyzeDataModel*>(pModel);
    connect(static_cast<ANSHAREDLIB::AnalyzeDataModel*>(pModel), &ANSHAREDLIB::AnalyzeDataModel::newFileAdded,
            this, &FilteringView::onNewFileLoaded);
}

//=============================================================================================================

void FilteringView::customMenuRequested(QPoint pos)
{
//    QMenu *menu = new QMenu(this);

//    QAction* pAction = new QAction("Remove", this);
//    connect(pAction, &QAction::triggered, [=]() {
//        emit removeItem(m_pUi->m_pTreeView->indexAt(pos));
//    });

//    menu->addAction(pAction);
//    menu->popup(m_pUi->m_pTreeView->viewport()->mapToGlobal(pos));
}

//=============================================================================================================

void FilteringView::onCurrentItemChanged(const QItemSelection &selected,
                                           const QItemSelection &deselected)
{
    Q_UNUSED(deselected)

    if(QStandardItemModel *pModel = qobject_cast<QStandardItemModel *>(m_pUi->m_pTreeView->model())) {        
        if(QStandardItem* pItem = pModel->itemFromIndex(selected.indexes().first())) {
            if(!pItem->data().isNull()) {
                emit selectedModelChanged(pItem->data());
            }
        }
    }
}

//=============================================================================================================

void FilteringView::onNewFileLoaded(int iSubject,
                                      int iModel)
{
    m_pUi->m_pTreeView->selectionModel()->select(m_pUi->m_pTreeView->model()->index(iModel, 0, m_pUi->m_pTreeView->model()->index(iSubject, 0)),
                                                 QItemSelectionModel::ClearAndSelect);
}
