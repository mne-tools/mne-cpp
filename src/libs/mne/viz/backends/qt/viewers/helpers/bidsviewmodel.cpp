//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2019-2026 MNE-CPP Authors
 *
 * @file     bidsviewmodel.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.4
 * @date     May 2019
 * @brief    Implementation of the BidsViewModel BIDS-tree model.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bidsviewmodel.h"
#include <mne/base/log.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtDebug>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz::qt;
using mne::viz::BidsTree;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BidsViewModel::BidsViewModel(QObject* pParent)
: QStandardItemModel(pParent)
{
    // Names edited in a view go back into the tree, so subjects are found by their displayed name
    connect(this, &QStandardItemModel::itemChanged, this, [this](QStandardItem* pItem) {
        const int node = pItem->data(BIDS_ITEM_NODE).isValid() ? pItem->data(BIDS_ITEM_NODE).toInt() : -1;
        if (m_tree.contains(node) && m_tree.node(node).name != pItem->text().toStdString()) {
            m_tree.rename(node, pItem->text().toStdString());
        }
    });
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
    switch (iDataType) {
        case BIDS_FUNCTIONALDATA:
        case BIDS_ANATOMICALDATA:
        case BIDS_BEHAVIORALDATA: {
            if (!selectedItem.isValid()) {
                addDataToSession(addSessionToSubject(addSubject("sub-01"), "ses-01"), pNewItem, iDataType);
            } else if (itemFromIndex(selectedItem)->data(BIDS_ITEM_TYPE).value<int>() != BIDS_SUBJECT) {
                addDataToSession(itemFromIndex(selectedItem)->data(BIDS_ITEM_SESSION).value<QModelIndex>(), pNewItem, iDataType);
            } else {
                mne::Log::debug("[BidsViewModel::addData] Prompt user to select a session");
            }
            break;
        }
        case BIDS_EVENT:
        case BIDS_AVERAGE:
        case BIDS_DIPOLE: {
            if (!selectedItem.isValid()) {
                auto pOwnedItem = std::make_unique<QStandardItem>("Unknown");
                pOwnedItem->setEditable(false);
                pOwnedItem->setDragEnabled(true);
                QStandardItem* pItem = pOwnedItem.get();
                addDataToSession(addSessionToSubject(addSubject("sub-01"), "ses-01"), pOwnedItem.release(), BIDS_UNKNOWN);
                addToData(pNewItem, indexFromItem(pItem), iDataType);
            } else {
                addToData(pNewItem, selectedItem, iDataType);
            }
            break;
        }
    }
}

//=============================================================================================================

void BidsViewModel::addToData(QStandardItem* pNewAvgItem,
                              const QModelIndex& parentIndex,
                              int iDataType)
{
    if (!m_tree.contains(nodeOf(parentIndex))) {
        mne::Log::warning("[BidsViewModel::addToData] The parent is not an item of this model");
        return;
    }

    QStandardItem* selectedData = itemFromIndex(parentIndex);
    const int node = m_tree.add_to_data(nodeOf(parentIndex), pNewAvgItem->text().toStdString(), iDataType);

    selectedData->setChild(selectedData->rowCount(), pNewAvgItem);
    tag(pNewAvgItem, node);

    emit newItemIndex(pNewAvgItem->index());
}

//=============================================================================================================

QModelIndex BidsViewModel::addSubject(const QString& sSubjectName)
{
    const int node = m_tree.add_subject(sSubjectName.toStdString());

    appendRow(new QStandardItem(QString::fromStdString(m_tree.node(node).name)));
    QStandardItem* pSubjectItem = item(rowCount() - 1);
    tag(pSubjectItem, node);

    emit newItemIndex(pSubjectItem->index());

    return pSubjectItem->index();
}

//=============================================================================================================

QModelIndex BidsViewModel::addSessionToSubject(const QString& sSubjectName,
                                               const QString& sSessionName)
{
    const std::vector<int> sessions = m_tree.add_session(sSubjectName.toStdString(), sSessionName.toStdString());

    if (sessions.empty()) {
        mne::Log::warning("[BidsViewModel::addSessionToSubject] No Subject found with name: {}", sSubjectName.toStdString());
        return QModelIndex();
    }
    if (sessions.size() > 1) {
        mne::Log::warning("[BidsViewModel::addSessionToSubject] Multiple subjects with same name");
    }

    QStandardItem* pNewSessionItem = nullptr;
    for (int session : sessions) {
        // Subjects with the same name were renamed by the tree
        QStandardItem* pSubjectItem = itemOf(m_tree.node(session).subject);
        pSubjectItem->setText(QString::fromStdString(m_tree.node(m_tree.node(session).subject).name));

        const int iRow = pSubjectItem->rowCount();
        pSubjectItem->setChild(iRow, new QStandardItem(QString::fromStdString(m_tree.node(session).name)));
        pNewSessionItem = pSubjectItem->child(iRow);
        tag(pNewSessionItem, session);

        emit newItemIndex(pNewSessionItem->index());
    }

    return pNewSessionItem ? pNewSessionItem->index() : QModelIndex();
}

//=============================================================================================================

QModelIndex BidsViewModel::addSessionToSubject(QModelIndex subjectIndex,
                                               const QString& sSessionName)
{
    if (!m_tree.contains(nodeOf(subjectIndex))) {
        mne::Log::warning("[BidsViewModel::addSessionToSubject] The subject is not an item of this model");
        return QModelIndex();
    }

    const int session = m_tree.add_session(nodeOf(subjectIndex), sSessionName.toStdString());

    QStandardItem* pSubjectItem = itemFromIndex(subjectIndex);
    const int iRow = pSubjectItem->rowCount();
    pSubjectItem->setChild(iRow, new QStandardItem(QString::fromStdString(m_tree.node(session).name)));
    QStandardItem* pNewSessionItem = pSubjectItem->child(iRow);
    tag(pNewSessionItem, session);

    emit newItemIndex(pNewSessionItem->index());

    return pNewSessionItem->index();
}

//=============================================================================================================

QModelIndex BidsViewModel::addDataToSession(QModelIndex sessionIndex,
                                            QStandardItem* pNewItem,
                                            int iDataType)
{
    if (!m_tree.contains(nodeOf(sessionIndex))) {
        mne::Log::warning("[BidsViewModel::addDataToSession] The session is not an item of this model");
        return QModelIndex();
    }

    const BidsTree::Placement placement = m_tree.add_data_to_session(nodeOf(sessionIndex), pNewItem->text().toStdString(), iDataType);

    QStandardItem* pSessionItem = itemFromIndex(sessionIndex);
    if (placement.folder_created) {
        auto pOwnedFolder = std::make_unique<QStandardItem>(QString::fromStdString(m_tree.node(placement.folder).name));
        QStandardItem* pFolder = pOwnedFolder.get();
        pSessionItem->setChild(pSessionItem->rowCount(), pOwnedFolder.release());
        tag(pFolder, placement.folder);
    }

    QStandardItem* pFolder = itemOf(placement.folder);
    pFolder->setChild(pFolder->rowCount(), pNewItem);
    tag(pNewItem, placement.item);

    emit newItemIndex(pNewItem->index());

    return pNewItem->index();
}

//=============================================================================================================

QModelIndex BidsViewModel::moveSessionToSubject(QModelIndex subjectIndex,
                                                QModelIndex sessionIndex)
{
    if (!m_tree.contains(nodeOf(subjectIndex)) || !m_tree.contains(nodeOf(sessionIndex))) {
        return QModelIndex();
    }

    beginResetModel();

    QStandardItem* subjectItem = itemFromIndex(subjectIndex);
    QStandardItem* sessionItem = itemFromIndex(sessionIndex);
    const int session = nodeOf(sessionIndex);

    m_tree.move_session(session, nodeOf(subjectIndex));

    sessionItem->parent()->takeRow(sessionItem->row());
    subjectItem->setChild(subjectItem->rowCount(), sessionItem);
    tag(sessionItem, session);

    endResetModel();

    emit newItemIndex(sessionItem->index());

    return sessionItem->index();
}

//=============================================================================================================

QModelIndex BidsViewModel::moveDataToSession(QModelIndex sessionIndex,
                                             QModelIndex dataIndex)
{
    if (!m_tree.contains(nodeOf(sessionIndex)) || !m_tree.contains(nodeOf(dataIndex))) {
        return QModelIndex();
    }

    beginResetModel();

    QStandardItem* dataItem = itemFromIndex(dataIndex);
    const int data = nodeOf(dataIndex);
    const BidsTree::Placement placement = m_tree.move_data(data, nodeOf(sessionIndex));

    // Take the item out and drop its folder when it was the last item in it
    QStandardItem* oldFolder = dataItem->parent();
    oldFolder->takeRow(dataItem->row());
    if (oldFolder->rowCount() == 0) {
        oldFolder->parent()->removeRow(oldFolder->row());
    }

    QStandardItem* pSessionItem = itemOf(m_tree.node(placement.folder).session);
    if (placement.folder_created) {
        auto pOwnedFolder = std::make_unique<QStandardItem>(QString::fromStdString(m_tree.node(placement.folder).name));
        QStandardItem* pFolder = pOwnedFolder.get();
        pSessionItem->setChild(pSessionItem->rowCount(), pOwnedFolder.release());
        tag(pFolder, placement.folder);
    }

    QStandardItem* pFolder = itemOf(placement.folder);
    pFolder->setChild(pFolder->rowCount(), dataItem);
    tag(dataItem, data);

    endResetModel();

    const QModelIndex newIndex = dataItem->index();
    emit newItemIndex(pSessionItem->index());
    emit newItemIndex(newIndex);

    return newIndex;
}

//=============================================================================================================

bool BidsViewModel::removeItem(QModelIndex itemIndex)
{
    if (!itemIndex.isValid()) {
        return false;
    }

    beginResetModel();

    QStandardItem* pItem = itemFromIndex(itemIndex);
    mne::Log::info("Deleting {}", pItem->text().toStdString());

    m_tree.remove(nodeOf(itemIndex));
    if (removeRows(itemIndex.row(), 1, itemIndex.parent())) {
        endResetModel();
    }

    return true;
}

//=============================================================================================================

int BidsViewModel::nodeOf(const QModelIndex& index) const
{
    const QStandardItem* pItem = itemFromIndex(index);
    return pItem && pItem->data(BIDS_ITEM_NODE).isValid() ? pItem->data(BIDS_ITEM_NODE).toInt() : -1;
}

//=============================================================================================================

QStandardItem* BidsViewModel::itemOf(int node) const
{
    QStandardItem* pItem = nullptr;
    for (int row : m_tree.path(node)) {
        pItem = pItem ? pItem->child(row) : item(row);
    }
    return pItem;
}

//=============================================================================================================

void BidsViewModel::tag(QStandardItem* pItem, int node)
{
    const BidsTree::Node& treeNode = m_tree.node(node);
    pItem->setData(QVariant::fromValue(node), BIDS_ITEM_NODE);
    pItem->setData(QVariant::fromValue(treeNode.type), BIDS_ITEM_TYPE);
    pItem->setData(QVariant::fromValue(indexFromItem(itemOf(treeNode.subject))), BIDS_ITEM_SUBJECT);
    if (treeNode.session >= 0) {
        pItem->setData(QVariant::fromValue(indexFromItem(itemOf(treeNode.session))), BIDS_ITEM_SESSION);
    }

    for (int row = 0; row < pItem->rowCount(); ++row) {
        if (QStandardItem* pChild = pItem->child(row); pChild && pChild->data(BIDS_ITEM_NODE).isValid()) {
            tag(pChild, pChild->data(BIDS_ITEM_NODE).toInt());
        }
    }
}
