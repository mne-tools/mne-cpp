//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file bidsviewmodel.h
 * @since October 2020
 * @brief QStandardItemModel mirroring the BIDS dataset hierarchy (subjects / sessions / runs / derivatives).
 *
 * BidsViewModel rebuilds itself from a top-level dataset path and
 * exposes the BIDS tree to @ref BidsView. Item roles carry the
 * absolute path, the BIDS entity type and the loadable-by-this-app
 * flag so the surrounding plugin can react to double-clicks (load
 * raw, attach derivative, …) without re-walking the file system.
 */

#ifndef BIDSVIEWMODEL_H
#define BIDSVIEWMODEL_H

//=============================================================================================================
// DEFINE MACROS
//=============================================================================================================

//USER ROLES
#define BIDS_ITEM_TYPE              Qt::UserRole+2
#define BIDS_ITEM_SUBJECT           Qt::UserRole+3
#define BIDS_ITEM_SESSION           Qt::UserRole+4

//ITEM TYPES

//CONTAINERS
#define BIDS_SUBJECT                01
#define BIDS_SESSION                02
#define BIDS_FOLDER                 03

//DATA TYPES
#define BIDS_FUNCTIONALDATA         10
#define BIDS_ANATOMICALDATA         11
#define BIDS_BEHAVIORALDATA         12
#define BIDS_IEEGDATA               13

//SUB-ITEMS
#define BIDS_AVERAGE                20
#define BIDS_EVENT             21
#define BIDS_DIPOLE                 22
#define BIDS_ELECTRODES             23
#define BIDS_COORDSYSTEM            24

#define BIDS_UNKNOWN                99

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QStandardItemModel>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE ANSHAREDLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// ANSHAREDLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// ENUMERATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * @brief QStandardItemModel mirroring the BIDS dataset hierarchy (subjects / sessions / runs / derivatives).
 *
 * Rebuilds itself from a top-level dataset path; item roles carry
 * the absolute path, BIDS entity type and loadable-by-this-app flag
 * so the host @ref BidsView can dispatch double-clicks without
 * re-walking the file system.
 */
class DISPSHARED_EXPORT BidsViewModel : public QStandardItemModel
{
    Q_OBJECT

public:
    typedef QSharedPointer<BidsViewModel> SPtr;               /**< Shared pointer type for BidsViewModel. */
    typedef QSharedPointer<const BidsViewModel> ConstSPtr;    /**< Const shared pointer type for BidsViewModel. */

    //=========================================================================================================
    /**
     * Constructs the Analyze Data Model.
     */
    BidsViewModel(QObject* pParent = Q_NULLPTR);

    //=========================================================================================================
    /**
     * Destroys the Analyze Data Model.
     */
    ~BidsViewModel();

public slots:
    //=========================================================================================================
    /**
     * Adds data to the item model.
     *
     * @param[in] sSubjectName          The subject name to store the data under.
     * @param[in] pItem                 The item to be added.
     */
    void addData(QModelIndex selectedItem,
                 QStandardItem *pNewItem,
                 int iDataType);

    //=========================================================================================================
    /**
     * Adds a sub-item to a data item (ex. an average)
     *
     * @param[in] pNewItem     item to be added.
     * @param[in] parentIndex  index of where the nitem should be added.
     */
    void addToData(QStandardItem *pNewItem,
                   const QModelIndex &parentIndex,
                   int iDataType);

    //=========================================================================================================
    /**
     * Adds subject with given input name
     *
     * @param[in] sSubjectName     name of subject name.
     *
     * @return  index of newly added item.
     */
    QModelIndex addSubject(const QString &sSubjectName);

    //=========================================================================================================
    /**
     * Adds session named after sSessionName to subjects names sSubjectName
     *
     * @param[in] sSubjectName     Name of subject to which session will be added.
     * @param[in] sSessionName     Name of session to add.
     *
     * @return  index of newly added item.
     */
    QModelIndex addSessionToSubject(const QString &sSubjectName,
                                    const QString &sSessionName);

    //=========================================================================================================
    /**
     * Adds session named after sSessionName to subject at subjectIndex
     *
     * @param[in] subjectIndex     Index of subject to which session will be added.
     * @param[in] sSessionName     Name of session to add.
     *
     * @return  index of newly added item.
     */
    QModelIndex addSessionToSubject(QModelIndex subjectIndex,
                                    const QString &sSessionName);

    //=========================================================================================================
    /**
     * Adds data item pNewItem to session at sessionIndex
     *
     * @param[in] sessionIndex     Index of session to which data will be added.
     * @param[in] pNewItem         New item to be added.
     *
     * @return index of newly added item.
     */
    QModelIndex addDataToSession(QModelIndex sessionIndex,
                                 QStandardItem* pNewItem,
                                 int iDataType);

    //=========================================================================================================
    /**
     * Moves a session to a new subject
     * @param[in] subjectIndex     Target subject.
     * @param[in] sessionIndex     Session to be moved.
     *
     * @return
     */
    QModelIndex moveSessionToSubject(QModelIndex subjectIndex,
                                     QModelIndex sessionIndex);

    //=========================================================================================================
    /**
     * Moves data at dataIndex to session at sessionIndex
     *
     * @param[in] sessionIndex     Target session.
     * @param[in] dataIndex        Data to be moved.
     *
     * @return
     */
    QModelIndex moveDataToSession(QModelIndex sessionIndex,
                                  QModelIndex dataIndex);

    //=========================================================================================================
    /**
     * Removes item at index itemIndex
     *
     * @param[in] itemIndex    index of item to be removed.
     *
     * @return true if succefully removed, false if not.
     */
    bool removeItem(QModelIndex itemIndex);

signals:

    //=========================================================================================================
    /**
     * Emit index of newly added items to be selected
     *
     * @param[in] itemIndex    index of new item.
     */
    void newItemIndex(QModelIndex itemIndex);

};

} //Namespace

#endif //BIDSVIEWMODEL_H
