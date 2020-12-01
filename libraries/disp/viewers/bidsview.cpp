//=============================================================================================================
/**
 * @file     bidsview.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.6
 * @date     October, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch, Lars Debor, Simon Heinke, Gabriel Motta. All rights reserved.
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
 * @brief    Contains the declaration of the BidsView class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bidsview.h"
#include "ui_bidsview.h"

#include "helpers/bidsviewmodel.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QTreeView>
#include <QStandardItemModel>
#include <QDebug>
#include <QMenu>
#include <QKeyEvent>
#include <QMessageBox>
#include <QInputDialog>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BidsView::BidsView(QWidget *parent)
: AbstractView(parent)
, m_pUi(new Ui::BidsViewWidget)
{
    m_pUi->setupUi(this);
    m_pUi->m_pTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_pUi->m_pTreeView, &QTreeView::customContextMenuRequested,
            this, &BidsView::customMenuRequested, Qt::UniqueConnection);
}

//=============================================================================================================

BidsView::~BidsView()
{
    delete m_pUi;
}

//=============================================================================================================

void BidsView::setModel(QAbstractItemModel *pModel)
{
    m_pUi->m_pTreeView->setModel(pModel);

    connect(m_pUi->m_pTreeView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &BidsView::onCurrentItemChanged, Qt::UniqueConnection);
//    connect(static_cast<ANSHAREDLIB::AnalyzeDataModel*>(pModel), &ANSHAREDLIB::AnalyzeDataModel::newFileAdded,
//            this, &DataManagerControlView::onNewFileLoaded);
    connect(static_cast<DISPLIB::BidsViewModel*>(pModel), &DISPLIB::BidsViewModel::newItemIndex,
            this, &BidsView::onNewItemIndex);

    //Connect BIDS Model Functions
    DISPLIB::BidsViewModel *pBidsModel = qobject_cast<DISPLIB::BidsViewModel *>(pModel);

    //Adding
    connect(this, &BidsView::onAddSubject,
            pBidsModel, &BidsViewModel::addSubject);
    connect(this, SIGNAL(onAddSession(QModelIndex, const QString&)),
            pBidsModel, SLOT(addSessionToSubject(QModelIndex, const QString&)));

    //Moving
    connect(this, &BidsView::onMoveSession,
            pBidsModel, &BidsViewModel::moveSessionToSubject);
    connect(this, &BidsView::onMoveData,
            pBidsModel, &BidsViewModel::moveDataToSession);

    //Updating
    connect(pBidsModel, &BidsViewModel::modelReset,
            this, &BidsView::onModelReset);
}

//=============================================================================================================

void BidsView::customMenuRequested(QPoint pos)
{
    QString sToolTip = m_pUi->m_pTreeView->model()->data(m_pUi->m_pTreeView->indexAt(pos), Qt::ToolTipRole).toString();

    DISPLIB::BidsViewModel *pModel = qobject_cast<DISPLIB::BidsViewModel *>(m_pUi->m_pTreeView->model());

    QAction* pRemoveAction;

    if(m_pUi->m_pTreeView->indexAt(pos).isValid()){
        QStandardItem* pItem = pModel->itemFromIndex(m_pUi->m_pTreeView->indexAt(pos));

        switch (pItem->data(BIDS_ITEM_TYPE).value<int>()){
            case BIDS_SUBJECT: {
                QMenu *menu = new QMenu(this);

                QAction* pAddSessionAction = new QAction("Add Session", this);
                connect(pAddSessionAction, &QAction::triggered, [=]() {
                    bool ok;
                    QString text = QInputDialog::getText(this, tr("Adding Session"),
                                                         tr("Please name new session:"), QLineEdit::Normal,
                                                         "", &ok);
                    if (ok && !text.isEmpty()){
                        emit onAddSession(pItem->index(), text);
                    }
                });

                pRemoveAction = new QAction("Remove Subject", this);

                menu->addAction(pAddSessionAction);
                menu->addAction(pRemoveAction);
                menu->popup(m_pUi->m_pTreeView->viewport()->mapToGlobal(pos));
                break;
            }
            case BIDS_SESSION: {
                QMenu *menu = new QMenu(this);

                pRemoveAction = new QAction("Remove Session", this);

                QMenu* pMoveMenu = new QMenu("Move Session to ...");

                //Find all available subjects
                for(int i = 0; i < pModel->rowCount(); i++){
                    if(pModel->item(i)->index() != pItem->data(BIDS_ITEM_SUBJECT).value<QModelIndex>()){
                        qDebug() << "Relative model index" << pModel->item(i)->index();
                        qDebug() << "Relative item index" << pItem->data(BIDS_ITEM_SUBJECT).value<QModelIndex>();
                        QAction* pTargetAction = new QAction(pModel->item(i)->text());
                        connect(pTargetAction, &QAction::triggered, [=] () {
                            emit onMoveSession(pModel->item(i)->index(), pItem->index());
                        });
                        pMoveMenu->addAction(pTargetAction);
                    }
                }

                if (!pMoveMenu->isEmpty()){
                    menu->addMenu(pMoveMenu);
                }
                menu->addAction(pRemoveAction);
                menu->popup(m_pUi->m_pTreeView->viewport()->mapToGlobal(pos));
                break;
            }
            case BIDS_BEHAVIORALDATA:
            case BIDS_ANATOMICALDATA:
            case BIDS_FUNCTIONALDATA: {
                QMenu *menu = new QMenu(this);

                pRemoveAction = new QAction("Remove Data", this);

                QMenu* pMoveMenu = new QMenu("Move Data to ...");

                //Find all available sessions
                for(int i = 0; i < pModel->rowCount(); i++){
                    QMenu* pSubjectMenu = new QMenu(pModel->item(i)->text());
                    for (int j = 0; j < pModel->item(i)->rowCount(); j++){
                        if(pModel->item(i)->child(j)->index() != pItem->data(BIDS_ITEM_SESSION).value<QModelIndex>()){
                            QAction* pTargetAction = new QAction(pModel->item(i)->child(j)->text());
                            connect(pTargetAction, &QAction::triggered, [=] {
                                emit onMoveData(pModel->item(i)->child(j)->index(), pItem->index());
                            });
                            pSubjectMenu->addAction(pTargetAction);
                        }
                    }
                    if(!pSubjectMenu->isEmpty()){
                        pMoveMenu->addMenu(pSubjectMenu);
                    }
                }

                if (!pMoveMenu->isEmpty()){
                    menu->addMenu(pMoveMenu);
                }
                menu->addAction(pRemoveAction);
                menu->popup(m_pUi->m_pTreeView->viewport()->mapToGlobal(pos));
                break;
            }
            default:{
                qDebug() << "DataManagerControlView::customMenuRequested - default";
                QMenu *menu = new QMenu(this);

                pRemoveAction = new QAction("Remove", this);
                menu->addAction(pRemoveAction);
                menu->popup(m_pUi->m_pTreeView->viewport()->mapToGlobal(pos));
                break;
            }
        }
        connect(pRemoveAction, &QAction::triggered, [=]() {
            QMessageBox msgBox;

            msgBox.setText("Are you sure you want to remove " + pItem->text() + "?");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::No);

            int ret = msgBox.exec();

            if(ret == QMessageBox::Yes) {
            emit removeItem(m_pUi->m_pTreeView->indexAt(pos));
            }
        });
    } else {
        QMenu *menu = new QMenu(this);

        QAction* pAddSubjectAction = new QAction("Add Subject", this);
        connect(pAddSubjectAction, &QAction::triggered, [=]() {
            bool ok;
            QString text = QInputDialog::getText(this, tr("Adding Subject"),
                                                 tr("Please name new subject:"), QLineEdit::Normal,
                                                 "", &ok);
            if (ok && !text.isEmpty()){
                emit onAddSubject(text);
            }
        });

        menu->addAction(pAddSubjectAction);
        menu->popup(m_pUi->m_pTreeView->viewport()->mapToGlobal(pos));
    }
}

//=============================================================================================================

void BidsView::onCurrentItemChanged(const QItemSelection &selected,
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

void BidsView::onNewFileLoaded(int iSubject,
                               int iModel)
{
    m_pUi->m_pTreeView->selectionModel()->select(m_pUi->m_pTreeView->model()->index(iModel, 0, m_pUi->m_pTreeView->model()->index(iSubject, 0)),
                                                 QItemSelectionModel::ClearAndSelect);
}

//=============================================================================================================

void BidsView::keyPressEvent(QKeyEvent *event)
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

void BidsView::onNewItemIndex(QModelIndex itemIndex)
{
    m_pUi->m_pTreeView->selectionModel()->select(itemIndex, QItemSelectionModel::ClearAndSelect);
    m_pUi->m_pTreeView->expand(itemIndex.parent());
//    m_pUi->m_pTreeView->expandRecursively(itemIndex);
    m_pUi->m_pTreeView->expandAll();
}

//=============================================================================================================

void BidsView::saveSettings()
{

}

//=============================================================================================================

void BidsView::loadSettings()
{

}

//=============================================================================================================

void BidsView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void BidsView::updateProcessingMode(ProcessingMode mode)
{
    switch(mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void BidsView::onModelReset()
{
    m_pUi->m_pTreeView->expandAll();
}

//=============================================================================================================

void BidsView::clearView()
{

}
