//=============================================================================================================
/**
 * @file     bidsviewmodel.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.6
 * @date     October, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch, Gabriel Motta. All rights reserved.
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
 * @brief    Definition of the BidsViewModel Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bidsviewmodel.h"
#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BidsViewModel::BidsViewModel(QObject *pParent)
: QStandardItemModel(pParent)
{

}

//=============================================================================================================

BidsViewModel::~BidsViewModel()
{
}

//=============================================================================================================

void BidsViewModel::addData(QModelIndex selectedItem,
                            QStandardItem* pNewItem,
                            int iDataType)
{
    switch(iDataType){
    case BIDS_FUNCTIONALDATA:
    case BIDS_ANATOMICALDATA:
    case BIDS_BEHAVIORALDATA: {
        if(!selectedItem.isValid()) {
            addDataToSession(addSessionToSubject(addSubject("sub-01"), "ses-01"), pNewItem, iDataType);
        } else {
            if (itemFromIndex(selectedItem)->data(BIDS_ITEM_TYPE).value<int>() != BIDS_SUBJECT){
                addDataToSession(itemFromIndex(selectedItem)->data(BIDS_ITEM_SESSION).value<QModelIndex>(),
                                 pNewItem,
                                 iDataType);
            } else {
                qDebug() << "[BidsViewModel::addData] Prompt user to select a session";
            }
        }
        break;
    }
    case BIDS_EVENT:
    case BIDS_AVERAGE: {
        if(!selectedItem.isValid()) {
            QStandardItem* pItem = new QStandardItem("Unknown");
            pItem->setEditable(false);
            pItem->setDragEnabled(true);

            addDataToSession(addSessionToSubject(addSubject("sub-01"), "ses-01"), pItem, BIDS_UNKNOWN);
            addToData(pNewItem,
                      indexFromItem(pItem),
                      iDataType);
        } else {
        addToData(pNewItem,
                  selectedItem,
                  iDataType);
        }
        break;
    }
    }
}

//=============================================================================================================

void BidsViewModel::addToData(QStandardItem *pNewAvgItem,
                              const QModelIndex &parentIndex,
                              int iDataType)
{
    QStandardItem* selectedData = itemFromIndex(parentIndex);
    selectedData->setChild(selectedData->rowCount(),
                           pNewAvgItem);

    pNewAvgItem->setData(itemFromIndex(parentIndex)->data(BIDS_ITEM_SUBJECT), BIDS_ITEM_SUBJECT);
    pNewAvgItem->setData(itemFromIndex(parentIndex)->data(BIDS_ITEM_SESSION), BIDS_ITEM_SESSION);
    pNewAvgItem->setData(QVariant::fromValue(iDataType), BIDS_ITEM_TYPE);

    emit newItemIndex(pNewAvgItem->index());
}

//=============================================================================================================

QModelIndex BidsViewModel::addSubject(const QString &sSubjectName)
{
    //Ensure subject name follow BIDS format
    QString sNewSubjectName;

    if(!sSubjectName.startsWith("sub-")){
        sNewSubjectName = "sub-" + sSubjectName;
    } else {
        sNewSubjectName = sSubjectName;
    }

    sNewSubjectName.remove(" ");

    //Add subject to model
    QStandardItem* pSubjectItem = new QStandardItem(sNewSubjectName);
    appendRow(pSubjectItem);

    pSubjectItem->setData(QVariant::fromValue(BIDS_SUBJECT), BIDS_ITEM_TYPE);
    pSubjectItem->setData(QVariant::fromValue(pSubjectItem->index()), BIDS_ITEM_SUBJECT);

    emit newItemIndex(pSubjectItem->index());

    return pSubjectItem->index();
}

//=============================================================================================================

QModelIndex BidsViewModel::addSessionToSubject(const QString &sSubjectName,
                                               const QString &sSessionName)
{
    //Ensure session name follow BIDS format
    QString sNewSessionName;

    if(!sSessionName.startsWith("ses-")){
        sNewSessionName = "ses-" + sSessionName;
    } else {
        sNewSessionName = sSessionName;
    }

    sNewSessionName.remove(" ");

    QList<QStandardItem*> listItems = findItems(sSubjectName);
    bool bRepeat = false;

    //Check for repeated names, no names
    if (listItems.size() > 1) {
        qWarning() << "[BidsViewModel::addSessionToSubject] Multiple subjects with same name";
        bRepeat = true;
    } else if (listItems.size() == 0) {
        qWarning() << "[BidsViewModel::addSessionToSubject] No Subject found with name:" << sSubjectName;
        return QModelIndex();
    }

    QStandardItem* pNewSessionItem;

    //Add session to subjects with mathcing names. Renames them if multiple.
    for (int i = 0; i < listItems.size(); i++){
        QStandardItem* pSubjectItem = listItems.at(i);

        if(bRepeat){
            pSubjectItem->setText(pSubjectItem->text() + QString::number(i + 1));
        }

        pNewSessionItem = new QStandardItem(sNewSessionName);
        pSubjectItem->setChild(pSubjectItem->rowCount(),
                               pNewSessionItem);

        pNewSessionItem->setData(QVariant::fromValue(BIDS_SESSION), BIDS_ITEM_TYPE);
        pNewSessionItem->setData(QVariant::fromValue(pSubjectItem->index()), BIDS_ITEM_SUBJECT);
        pNewSessionItem->setData(QVariant::fromValue(pNewSessionItem->index()), BIDS_ITEM_SESSION);

        emit newItemIndex(pNewSessionItem->index());
    }

    return pNewSessionItem->index();
}

//=============================================================================================================

QModelIndex BidsViewModel::addSessionToSubject(QModelIndex subjectIndex,
                                               const QString &sSessionName)
{
    //Ensure session name follow BIDS format
    QString sNewSessionName;

    if(!sSessionName.startsWith("ses-")){
        sNewSessionName = "ses-" + sSessionName;
    } else {
        sNewSessionName = sSessionName;
    }

    sNewSessionName.remove(" ");

    QStandardItem* pSubjectItem = itemFromIndex(subjectIndex);
    QStandardItem* pNewSessionItem = new QStandardItem(sNewSessionName);
    pSubjectItem->setChild(pSubjectItem->rowCount(),
                           pNewSessionItem);

    pNewSessionItem->setData(QVariant::fromValue(BIDS_SESSION), BIDS_ITEM_TYPE);
    pNewSessionItem->setData(QVariant::fromValue(subjectIndex), BIDS_ITEM_SUBJECT);
    pNewSessionItem->setData(QVariant::fromValue(pNewSessionItem->index()), BIDS_ITEM_SESSION);

    emit newItemIndex(pNewSessionItem->index());

    return pNewSessionItem->index();
}

//=============================================================================================================

QModelIndex BidsViewModel::addDataToSession(QModelIndex sessionIndex,
                                            QStandardItem *pNewItem,
                                            int iDataType)
{
    QStandardItem* pSessionItem = itemFromIndex(sessionIndex);
    bool bFolder = false;
    int iFolder;

    QString sFolderName;

    switch (iDataType){
        case BIDS_FUNCTIONALDATA:
            sFolderName = "func";
            break;
        case BIDS_ANATOMICALDATA:
            sFolderName = "anat";
            break;
        case BIDS_BEHAVIORALDATA:
            sFolderName = "beh";
        default:
            sFolderName = "unknown";
    }

    for(iFolder = 0; iFolder < pSessionItem->rowCount(); iFolder++){
        if (pSessionItem->child(iFolder)->text() == sFolderName){
            bFolder = true;
            break;
        }
    }

    if(!bFolder) {
        QStandardItem* pFunctionalItem = new QStandardItem(sFolderName);
        pFunctionalItem->setData(QVariant::fromValue(BIDS_FOLDER), BIDS_ITEM_TYPE);
        pFunctionalItem->setData(QVariant::fromValue(sessionIndex), BIDS_ITEM_SESSION);
        pFunctionalItem->setData(itemFromIndex(sessionIndex)->data(BIDS_ITEM_SUBJECT), BIDS_ITEM_SUBJECT);

        pSessionItem->setChild(pSessionItem->rowCount(),
                               pFunctionalItem);
        pFunctionalItem->setChild(pFunctionalItem->rowCount(),
                           pNewItem);
    } else {
        pSessionItem->child(iFolder)->setChild(pSessionItem->child(iFolder)->rowCount(),
                                                  pNewItem);
    }

    pNewItem->setData(QVariant::fromValue(iDataType), BIDS_ITEM_TYPE);
    pNewItem->setData(QVariant::fromValue(sessionIndex), BIDS_ITEM_SESSION);
    pNewItem->setData(itemFromIndex(sessionIndex)->data(BIDS_ITEM_SUBJECT), BIDS_ITEM_SUBJECT);

    emit newItemIndex(pNewItem->index());

    return pNewItem->index();
}

//=============================================================================================================

QModelIndex BidsViewModel::moveSessionToSubject(QModelIndex subjectIndex,
                                                QModelIndex sessionIndex)
{
    beginResetModel();

    QStandardItem* subjectItem = itemFromIndex(subjectIndex);
    QStandardItem* sessionItem = itemFromIndex(sessionIndex);

    sessionItem->parent()->takeRow(sessionItem->row());

    subjectItem->setChild(subjectItem->rowCount(), sessionItem);
    subjectItem->setData(subjectIndex, BIDS_ITEM_SUBJECT);

    for (int i = 0; i < sessionItem->rowCount(); i++){
        sessionItem->child(i)->setData(subjectIndex, BIDS_ITEM_SUBJECT);
        for (int j = 0; j < sessionItem->child(i)->rowCount(); j++) {
            sessionItem->child(i)->child(j)->setData(subjectIndex, BIDS_ITEM_SUBJECT);
        }
    }

    endResetModel();

    emit newItemIndex(sessionItem->index());

    return sessionItem->index();
}

//=============================================================================================================

QModelIndex BidsViewModel::moveDataToSession(QModelIndex sessionIndex,
                                             QModelIndex dataIndex)
{
    beginResetModel();

    QStandardItem* dataItem = itemFromIndex(dataIndex);

    if(dataItem->parent()->rowCount() < 2){
        QStandardItem* parent = dataItem->parent();

        dataItem->parent()->takeRow(dataItem->row()).first();
        parent->parent()->takeRow(parent->row()).first();
    } else {
        dataItem->parent()->takeRow(dataItem->row()).first();
    }

    QModelIndex newIndex;
//    switch(dataItem->data(ITEM_TYPE).value<int>()){
//        case FUNCTIONALDATA:{
//            newIndex = addDataToSession(sessionIndex,
//                                        dataItem,
//                                        FUNCTIONALDATA);
//            break;
//        }
//        default:{
//            qWarning() << "[BidsViewModel::moveDataToSession] Move not supported for this type of data";
//        }
//    }

    newIndex = addDataToSession(sessionIndex,
                                dataItem,
                                dataItem->data(BIDS_ITEM_TYPE).value<int>());

    endResetModel();

    emit newItemIndex(sessionIndex);
    emit newItemIndex(newIndex);

    return newIndex;
}

//=============================================================================================================

bool BidsViewModel::removeItem(QModelIndex itemIndex)
{
    if(!itemIndex.isValid()){
        return false;
    }

    beginResetModel();

    QStandardItem* pItem = itemFromIndex(itemIndex);

    qInfo() << "Deleting" << pItem->text();

    switch(pItem->data(BIDS_ITEM_TYPE).value<int>()){
        case BIDS_SUBJECT:
            if(removeRows(itemIndex.row(), 1, itemIndex.parent())){
                endResetModel();
            }
            return true;
        case BIDS_SESSION:
            if(removeRows(itemIndex.row(), 1, itemIndex.parent())){
                endResetModel();
            }
            return true;
        case BIDS_BEHAVIORALDATA:
        case BIDS_ANATOMICALDATA:
        case BIDS_FUNCTIONALDATA:
            if(removeRows(itemIndex.row(), 1, itemIndex.parent())){
                endResetModel();
            }
            return true;
        case BIDS_AVERAGE:
        case BIDS_EVENT:
        case BIDS_DIPOLE:
            if(removeRows(itemIndex.row(), 1, itemIndex.parent())){
                endResetModel();
            }
            return true;
        default:
            if(removeRows(itemIndex.row(), 1, itemIndex.parent())){
                endResetModel();
            }
            return true;
    }
}
