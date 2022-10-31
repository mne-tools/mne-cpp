//=============================================================================================================
/**
 * @file     bemdatamodel.h
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.6
 * @date     August, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Ruben Dörfel. All rights reserved.
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
 * @brief     BemDataModel class declaration.
 *
 */

#ifndef ANSHAREDLIB_BEMDATAMODEL_H
#define ANSHAREDLIB_BEMDATAMODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../anshared_global.h"
#include "../Utils/types.h"
#include "abstractmodel.h"

#include <fiff/fiff_io.h>

#include <mne/mne_bem.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QSharedPointer>
#include <QBuffer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNELIB {
    class MNEBem;
}

//=============================================================================================================
// DEFINE NAMESPACE ANSHAREDLIB
//=============================================================================================================

namespace ANSHAREDLIB {

//=============================================================================================================
// ANSHAREDLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 *
 * @brief Model that holds and manages bem data.
 */
class ANSHAREDSHARED_EXPORT BemDataModel : public AbstractModel
{
    Q_OBJECT

public:
    typedef QSharedPointer<BemDataModel> SPtr;            /**< Shared pointer type for BemDataModel. */
    typedef QSharedPointer<const BemDataModel> ConstSPtr; /**< Const shared pointer type for BemDataModel. */

    //=========================================================================================================
    /**
    * Constructs a BemDataModel object.
    */
    explicit BemDataModel(QObject *parent = Q_NULLPTR);

    //=========================================================================================================
    /**
     * Constructs a BemDataModel object.
     *
     * @param[in] sFilePath             The file path of the model. This is usually also the file path.
     * @param[in] pParent               The parent model. Default is set to NULL.
     */
    BemDataModel(const QString &sFilePath,
                 const QByteArray& byteLoadedData = QByteArray(),
                 QObject *pParent = Q_NULLPTR);

    //=========================================================================================================
    /**
     * Destructs a BemDataModel.
     */
    ~BemDataModel() override;

    //=========================================================================================================
    /**
     * The type of this model (BemDataModel)
     *
     * @return The type of this model (BemDataModel).
     */
    inline MODEL_TYPE getType() const override;

    //=========================================================================================================
    /**
     * Get the Bem model (BemDataModel)
     *
     * @return The MNEBem object.
     */
    inline MNELIB::MNEBem::SPtr getBem();

    //=========================================================================================================
    /**
     * Returns whether there is currently data stored in the model
     *
     * @return Returns false if model is empty.
     */
    inline bool isEmpty() const;

    //=========================================================================================================
    // Inherited by QAbstractItemModel:
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    virtual QModelIndex parent(const QModelIndex &index) const override;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;

protected:

private:

    //=========================================================================================================
    /**
     * Helper function for initialization
     */
    void initBemData(QIODevice& p_IODevice);

    QFile                   m_file;                 /**< The IO file. */
    QByteArray              m_byteLoadedData;
    QBuffer                 m_buffer;

    MNELIB::MNEBem::SPtr    m_pBem;                 /**< Data. */

signals:
    //=========================================================================================================
    /**
     * Emits new Bem Model
     *
     * @param[in] pBem    The new Bem model.
     */
    void newBemAvailable(const MNELIB::MNEBem::SPtr pBem);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline MODEL_TYPE BemDataModel::getType() const
{
    return MODEL_TYPE::ANSHAREDLIB_BEMDATA_MODEL;
}

inline bool BemDataModel::isEmpty() const
{
    return m_pBem.isNull();
}

inline MNELIB::MNEBem::SPtr BemDataModel::getBem()
{
    return m_pBem;
}

} // namespace ANSHAREDLIB

#endif // ANSHAREDLIB_BEMDATAMODEL_H

