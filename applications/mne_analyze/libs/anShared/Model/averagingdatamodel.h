//=============================================================================================================
/**
 * @file     averagingdatamodel.h
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.6
 * @date     September, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Gabriel Motta. All rights reserved.
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
 * @brief    Declaration of the AveragingDataModel Class.
 *
 */


#ifndef AVERAGINGDATAMODEL_H
#define AVERAGINGDATAMODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../anshared_global.h"
#include "../Utils/types.h"

#include <fiff/fiff_info.h>

#include "abstractmodel.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QColor>
#include <QListWidgetItem>
#include <QStack>

//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffEvokedSet;
}

//=============================================================================================================
// DEFINE NAMESPACE ANSHAREDLIB
//=============================================================================================================

namespace ANSHAREDLIB {

//=============================================================================================================
/**
 * Model that holds averaging data in the form of a FiffEvokedSet
 */
class ANSHAREDSHARED_EXPORT AveragingDataModel : public AbstractModel
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a AveragingDataModel object
     *
     * @param[in] pEvokedSet   sets saved evoked set to pEvokedSet. NULL if left empty.
     * @param[in] parent       sets parent of the object. NULL if left empty.
     */
    AveragingDataModel(QSharedPointer<FIFFLIB::FiffEvokedSet> pEvokedSet = Q_NULLPTR,
                       QObject* parent = Q_NULLPTR);

    //=========================================================================================================
    AveragingDataModel(const QString &sFilePath,
                       const QByteArray& byteLoadedData = QByteArray(),
                       QObject* parent = Q_NULLPTR);

    //=========================================================================================================
    /**
     *  Destructs the AveragingDataModel object
     */
    ~AveragingDataModel();

    //=========================================================================================================
    /**
     * Returns the FiffEvokedSet stored in the model
     *
     * @param[in] index    index of stored data (all indeces retrun the same data).
     * @param[in] role     role of stored data (Unused).
     *
     * @return      returns saved FiffEvokedModel with averaging data.
     */
    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const override;

    //=========================================================================================================
    /**
     * Returns the item flags for the given index.
     *
     * @param[in] index   The index that referres to the requested item.
     */
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    //=========================================================================================================
    /**
     * Returns the number of rows in the model
     *
     * @param[in] parent     The parent index.
     */
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    //=========================================================================================================
    /**
     * Returns the number of columns in the model
     *
     * @param[in] parent     The parent index.
     */
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    //=========================================================================================================
    /**
     * Set the saved evoked set to pEvokedSet
     *
     * @param[in] pEvokedSet   new evoked set to be saved.
     */
    void setEvokedSet(QSharedPointer<FIFFLIB::FiffEvokedSet> pEvokedSet);

    //=========================================================================================================
    /**
     * Returns the stored FiffEvokedSet
     *
     * @return the stored FiffEvokedSet.
     */
    QSharedPointer<FIFFLIB::FiffEvokedSet> getEvokedSet();

    //=========================================================================================================
    /**
     * The type of this model (MriCoordModel)
     *
     * @return The type of this model (MriCoordModel).
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

    //=========================================================================================================
    /**
     * Returns whether averaging model was loaded from a file
     *
     * @return
     */
    bool isFromFile();

    //=========================================================================================================
    /**
     * Gets FiffInfo of the evoked model. To be used if loading avg from file without raw file present
     *
     * @return  Shared pointer to FiffInfo of m_pFiffEvokedSet.
     */
    QSharedPointer<FIFFLIB::FiffInfo> getFiffInfo();

private:
    QSharedPointer<FIFFLIB::FiffEvokedSet> m_pFiffEvokedSet; /**<  Pointer to FiffEvokedData for the calculated average*/

    bool m_bFromFile;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline MODEL_TYPE AveragingDataModel::getType() const
{
    return MODEL_TYPE::ANSHAREDLIB_AVERAGING_MODEL;
}

//=============================================================================================================

QModelIndex AveragingDataModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

//=============================================================================================================

QModelIndex AveragingDataModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return createIndex(row, column);
}

} //Namespace
#endif // AVERAGINGDATAMODEL_H
