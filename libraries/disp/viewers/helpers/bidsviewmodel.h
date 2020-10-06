//=============================================================================================================
/**
 * @file     analyzedatamodel.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.2
 * @date     May, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Lorenz Esch. All rights reserved.
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
 * @brief    Contains declaration of AnalyzeDataModel Container class.
 *
 */

#ifndef ANALYZEDATAMODEL_H
#define ANALYZEDATAMODEL_H

#define ITEM_TYPE Qt::UserRole+2
#define ITEM_SUBJECT Qt::UserRole+3
#define ITEM_SESSION Qt::UserRole+4

#define SUBJECT 1
#define SESSION 2
#define DATA 3
#define AVG 4


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
 * DECLARE CLASS AnalyzeDataModel
 *
 * @brief The AnalyzeDataModel class is the base data container.
 */
class DISPSHARED_EXPORT BidsViewModel : public QStandardItemModel
{
    Q_OBJECT

public:
    typedef QSharedPointer<BidsViewModel> SPtr;               /**< Shared pointer type for AnalyzeDataModel. */
    typedef QSharedPointer<const BidsViewModel> ConstSPtr;    /**< Const shared pointer type for AnalyzeDataModel. */

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

    //=========================================================================================================
    /**
     * Adds data to the item model.
     *
     * @param[in] sSubjectName          The subject name to store the data under.
     * @param[in] pItem                 The item to be added.
     */
    void addData(const QString &sSubjectName,
                 QStandardItem *pNewItem);

    //=========================================================================================================
    /**
     * Adds a sub-item to a data item (ex. an average)
     *
     * @param [in] pNewItem     item to be added
     * @param [in] parentIndex  index of where the nitem should be added
     */
    void addItemToData(QStandardItem *pNewItem,
                       const QModelIndex &parentIndex);

    //=========================================================================================================
    /**
     * Adds subject with given input name
     *
     * @param [in] sSubjectName     name of subject name
     *
     * @return  returns true if successful, false if not
     */
    bool addSubject(const QString &sSubjectName);

    //=========================================================================================================
    /**
     * Adds session named after sSessionName to subjects names sSubjectName
     *
     * @param [in] sSubjectName     Name of subject to which session will be added
     * @param [in] sSessionName     Name of session to add
     *
     * @return  returns true if successful, false if not
     */
    bool addSessionToSubject(const QString &sSubjectName,
                             const QString &sSessionName);

    //=========================================================================================================
    /**
     * Adds session named after sSessionName to subject at subjectIndex
     *
     * @param [in] subjectIndex     Index of subject tow which session will be added
     * @param [in] sSessionName     Name of session to add
     *
     * @return  returns true if successful, false if not
     */
    bool addSessionToSubject(QModelIndex subjectIndex,
                             const QString &sSessionName);

signals:

    //=========================================================================================================
    /**
     * Emit index of newly added items to be selected
     *
     * @param [in] itemIndex    index of new item
     */
    void newItemIndex(QModelIndex itemIndex);

};

} //Namespace

#endif //ANALYZEDATAMODEL_H
