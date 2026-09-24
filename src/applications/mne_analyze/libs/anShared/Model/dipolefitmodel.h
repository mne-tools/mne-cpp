//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     dipolefitmodel.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.7
 * @date     November, 2020
 * @brief    Declaration of the DipoleFitModel Class.
 */

#ifndef DIPOLEFITMODEL_H
#define DIPOLEFITMODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../anshared_global.h"
#include "../Utils/types.h"
#include "abstractmodel.h"

#include <inv/dipole_fit/inv_ecd_set.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE ANSHAREDLIB
//=============================================================================================================

namespace ANSHAREDLIB {

//=============================================================================================================
class ANSHAREDSHARED_EXPORT DipoleFitModel : public AbstractModel
{
    Q_OBJECT
public:
    typedef QSharedPointer<DipoleFitModel> SPtr;              /**< Shared pointer type for DipoleFitModel. */
    typedef QSharedPointer<const DipoleFitModel> ConstSPtr;   /**< Const shared pointer type for DipoleFitModel. */

public:
    //=========================================================================================================
    DipoleFitModel(const QString &sFilePath,
                   const QByteArray& byteLoadedData = QByteArray(),
                   QObject* parent = Q_NULLPTR);

    //=========================================================================================================
    DipoleFitModel(INVLIB::InvEcdSet InvEcdSet,
                   const QString &sFilePath = "",
                   const QByteArray& byteLoadedData = QByteArray(),
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

    INVLIB::InvEcdSet      m_ECD_Set;

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline MODEL_TYPE DipoleFitModel::getType() const
{
    return MODEL_TYPE::ANSHAREDLIB_DIPOLEFIT_MODEL;
}

//=============================================================================================================

QModelIndex DipoleFitModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

//=============================================================================================================

QModelIndex DipoleFitModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return createIndex(row, column);
}

}//namespace

#endif // DIPOLEFITMODEL_H
