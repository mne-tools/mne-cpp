//=============================================================================================================
/**
 * @file     bidsviewmodel.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
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
 * @brief    Contains declaration of BidsViewModel Container class.
 *
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

//SUB-ITEMS
#define BIDS_AVERAGE                20
#define BIDS_EVENT             21
#define BIDS_DIPOLE                 22

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

//=========================================================================================================
/**
 * DECLARE CLASS BidsViewModel
 *
 * @brief The BidsViewModel class is the base data container.
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
