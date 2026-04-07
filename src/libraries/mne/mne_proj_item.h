//=============================================================================================================
/**
 * @file     mne_proj_item.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    MNEProjItem class declaration.
 *
 */

#ifndef MNEPROJITEM_H
#define MNEPROJITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_named_matrix.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
/**
 * @brief A single SSP (Signal-Space Projection) item.
 *
 * MNEProjItem holds one linear projection operator consisting of a set of
 * projection vectors (stored as an MNENamedMatrix), together with metadata
 * such as the projection kind, a human-readable description, and flags
 * indicating whether the item is currently active and which channel types
 * (MEG / EEG) it covers.
 *
 * Projection items are aggregated by MNEProjOp to form the complete SSP
 * operator that is applied during inverse computations and dipole fitting.
 */
class MNESHARED_EXPORT MNEProjItem
{
public:
    typedef QSharedPointer<MNEProjItem> SPtr;              /**< Shared pointer type for MNEProjItem. */
    typedef QSharedPointer<const MNEProjItem> ConstSPtr;   /**< Const shared pointer type for MNEProjItem. */

    //=========================================================================================================
    /**
     * @brief Default constructor.
     *
     * Initialises all flags to their default values (active = true,
     * kind = FIFFV_PROJ_ITEM_NONE, no MEG/EEG channels).
     */
    MNEProjItem();

    //=========================================================================================================
    /**
     * @brief Copy constructor.
     *
     * Deep-copies the projection vectors (unique_ptr member).
     */
    MNEProjItem(const MNEProjItem& other);

    //=========================================================================================================
    /**
     * @brief Copy assignment operator.
     *
     * Deep-copies the projection vectors (unique_ptr member).
     */
    MNEProjItem& operator=(const MNEProjItem& other);

    //=========================================================================================================
    /**
     * @brief Move constructor (defaulted).
     */
    MNEProjItem(MNEProjItem&&) noexcept = default;

    //=========================================================================================================
    /**
     * @brief Move assignment operator (defaulted).
     */
    MNEProjItem& operator=(MNEProjItem&&) noexcept = default;

    //=========================================================================================================
    /**
     * @brief Destructor.
     */
    ~MNEProjItem();

    //=========================================================================================================
    /**
     * @brief Test whether this projection item affects any of the listed channels.
     *
     * Iterates over @p list and checks if any channel name matches a column
     * in the projection vectors that contains at least one non-zero entry.
     *
     * @param[in] list    Channel names to test against.
     * @param[in] nlist   Number of channel names in @p list.
     *
     * @return @c true (non-zero) if this item affects at least one channel,
     *         @c false (0) otherwise.
     */
    int affect(const QStringList& list, int nlist) const;

public:
    std::unique_ptr<MNENamedMatrix> vecs; /**< Projection vectors (nrow = nvec, ncol = nch); may be nullptr when nvec == 0. */
    int             nvec;           /**< Number of projection vectors (== vecs->nrow when vecs is set). */
    QString         desc;           /**< Human-readable description (e.g. "PCA-v1"). */
    int             kind;           /**< FIFF projection item kind (FIFFV_PROJ_ITEM_*). */
    bool            active;         /**< Whether this item is currently active. */
    bool            active_file;    /**< Whether this item was active when loaded from file. */
    bool            has_meg;        /**< Whether the projection covers MEG channels. */
    bool            has_eeg;        /**< Whether the projection covers EEG channels. */
};

} // NAMESPACE MNELIB

#endif // MNEPROJITEM_H
