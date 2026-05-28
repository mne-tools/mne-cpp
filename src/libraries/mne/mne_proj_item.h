//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_proj_item.h
 * @since 2026
 * @date  April 2026
 * @brief Single SSP projection vector with kind/active flag and channel labels.
 *
 * @ref MNELIB::MNEProjItem corresponds to one @c FIFFB_PROJ_ITEM block:
 * the labelled vector that gets removed from the data when the
 * projection is active. Used both by @ref FIFFLIB::FiffInfo's SSP list
 * and by @ref MNEProjOp when assembling the full projector.
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
