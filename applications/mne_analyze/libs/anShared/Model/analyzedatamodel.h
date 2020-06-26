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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../anshared_global.h"

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

namespace ANSHAREDLIB
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
class ANSHAREDSHARED_EXPORT AnalyzeDataModel : public QStandardItemModel
{
    Q_OBJECT

public:
    typedef QSharedPointer<AnalyzeDataModel> SPtr;               /**< Shared pointer type for AnalyzeDataModel. */
    typedef QSharedPointer<const AnalyzeDataModel> ConstSPtr;    /**< Const shared pointer type for AnalyzeDataModel. */

    //=========================================================================================================
    /**
     * Constructs the Analyze Data Model.
     */
    AnalyzeDataModel(QObject* pParent = Q_NULLPTR);

    //=========================================================================================================
    /**
     * Destroys the Analyze Data Model.
     */
    ~AnalyzeDataModel();

    //=========================================================================================================
    /**
     * Adds data to the item model.
     *
     * @param[in] sSubjectName          The subject name to store the data under.
     * @param[in] pItem                 The item to be added.
     */
    void addData(const QString &sSubjectName,
                 QStandardItem *pNewItem);

signals:
    //=========================================================================================================
    /**
     * Send index of newly added file model and index of subject it was added to
     *
     * @param [in] iSubjectIndex        index of subject that the new model was added to
     * @param [in] iChildModelIndex     index of new model
     */
    void newFileAdded(int iSubjectIndex, int iChildModelIndex);

};

} //Namespace

#endif //ANALYZEDATAMODEL_H
