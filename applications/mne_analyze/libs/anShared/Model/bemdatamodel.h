//=============================================================================================================
/**
 * @file     bemdatamodel.h
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.0
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
 * @brief Model that holds and manages bem data..
 */
class BemDataModel : public AbstractModel
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
     * Constructs a FiffRawViewModel object.
     *
     * @param[in] sFilePath             The file path of the model. This is usually also the file path.
     * @param[in] pParent               The parent model. Default is set to NULL.
     */
    BemDataModel(const QString &sFilePath,
                 QObject *pParent = Q_NULLPTR);
    //=========================================================================================================
    /**
     * Destructs a BemDataModel.
     */
    ~BemDataModel() override;

    //=========================================================================================================
    /**
     * The type of this model (FiffRawViewModel)
     *
     * @return The type of this model (FiffRawViewModel)
     */
    inline MODEL_TYPE getType() const override;

    //=========================================================================================================
    /**
     * Returns whether there is currently data stored in the model
     *
     * @return Returns false if model is empty
     */
    inline bool isEmpty() const;


protected:

private:

    std::list<QSharedPointer<MNELIB::MNEBem>>   m_lBem;             /**< Data */

signals:
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
    return m_lBem.empty();
}

} // namespace ANSHAREDLIB

#endif // ANSHAREDLIB_BEMDATAMODEL_H

