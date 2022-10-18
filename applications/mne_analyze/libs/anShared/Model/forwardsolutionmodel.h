//=============================================================================================================
/**
 * @file     forwardsolutionmodel.h
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.9
 * @date     October, 2022
 *
 * @section  LICENSE
 *
 * Copyright (C) 2022, Gabriel Motta. All rights reserved.
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
 * @brief    Declaration of the ForwardSolutionModel Class.
 *
 */

#ifndef FORWARDSOLUTIONMODEL_H
#define FORWARDSOLUTIONMODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../anshared_global.h"
#include "../Utils/types.h"
#include "abstractmodel.h"

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNELIB {
    class MNEForwardSolution;
}

//=============================================================================================================
// DEFINE NAMESPACE ANSHAREDLIB
//=============================================================================================================

namespace ANSHAREDLIB {

//=============================================================================================================
class ANSHAREDSHARED_EXPORT ForwardSolutionModel : public AbstractModel
{
    Q_OBJECT
public:
    typedef QSharedPointer<ForwardSolutionModel> SPtr;              /**< Shared pointer type for ForwardSolutionModel. */
    typedef QSharedPointer<const ForwardSolutionModel> ConstSPtr;   /**< Const shared pointer type for ForwardSolutionModel. */

public:
    //=========================================================================================================
    ForwardSolutionModel(QObject* parent = Q_NULLPTR);

    //=========================================================================================================
    ForwardSolutionModel(QSharedPointer<MNELIB::MNEForwardSolution> pFwdSolution,
                         QObject* parent = Q_NULLPTR);

    //=========================================================================================================
    /**
     * Returns the number of rows in the model
     *
     * @param[in] parent     The parent index.
     */
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    //=========================================================================================================
    /**
     * Returns the number of columns in the model
     *
     * @param[in] parent     The parent index.
     */
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    //=========================================================================================================
    /**
     * Returns the data stored under the given role for the index.
     *
     * @param[in] index   The index that referres to the requested item.
     * @param[in] role    The requested role.
     */
    virtual QVariant data(const QModelIndex &index,
                          int role = Qt::DisplayRole) const override;

    //=========================================================================================================
    /**
     * Returns the item flags for the given index.
     *
     * @param[in] index   The index that referres to the requested item.
     */
    Qt::ItemFlags flags(const QModelIndex & index) const override;

    //=========================================================================================================
    /**
     * The type of this model (CovarianceModel)
     *
     * @return The type of this model (CovarianceModel).
     */
    inline MODEL_TYPE getType() const override;

    //=========================================================================================================
    /**
     * Returns the index for the item in the model specified by the given row, column and parent index.
     * Currently only Qt::DisplayRole is supported.
     * Index rows reflect channels, first column is channel names, second is raw data.
     *
     * @param[in] row      The specified row.
     * @param[in] column   The specified column.
     * @param[in] parent   The parent index.
     */
    inline QModelIndex index(int row,
                             int column,
                             const QModelIndex &parent = QModelIndex()) const override;

    //=========================================================================================================
    /**
     * Returns the parent index of the given index.
     * In this Model the parent index in always QModelIndex().
     *
     * @param[in] index   The index that referres to the child.
     */
    inline QModelIndex parent(const QModelIndex &index) const override;

private:

    QSharedPointer<MNELIB::MNEForwardSolution> m_pFwdSolution;

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline MODEL_TYPE ForwardSolutionModel::getType() const
{
    return MODEL_TYPE::ANSHAREDLIB_FORWARDSOLUTION_MODEL;
}

//=============================================================================================================

QModelIndex ForwardSolutionModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

//=============================================================================================================

QModelIndex ForwardSolutionModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return createIndex(row, column);
}

}//namespace

#endif // FORWARDSOLUTIONMODEL_H
