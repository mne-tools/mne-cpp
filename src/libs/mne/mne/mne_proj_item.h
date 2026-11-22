//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     mne_proj_item.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     January 2017
 * @brief    Single SSP projection vector with kind/active flag and channel labels.
 *
 * @ref mne::MNEProjItem corresponds to one @c FIFFB_PROJ_ITEM block:
 * the labelled vector that gets removed from the data when the
 * projection is active. Used both by @ref mne::FiffInfo's SSP list
 * and by @ref mne::MNEProjOp when assembling the full projector.
 */

#ifndef MNEPROJITEM_H
#define MNEPROJITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_named_matrix.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <span>
#include <string>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
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
 *
 * @snippet ex_mne_api/main.cpp mne_raw_data_usage
 */
class MNESHARED_EXPORT MNEProjItem
{
public:
    //=========================================================================================================
    /**
     * @return The number of projection vectors (rows of @ref vecs).
     */
    int nvec() const
    {
        return vecs.nrow();
    }

    //=========================================================================================================
    /**
     * @brief Test whether this projection item affects any of the listed channels.
     *
     * Iterates over @p list and checks if any channel name matches a column
     * in the projection vectors that contains at least one non-zero entry.
     *
     * @param[in] list    Channel names to test against.
     *
     * @return Whether this item affects at least one channel.
     */
    bool affect(std::span<const std::string> list) const;

public:
    MNENamedMatrix vecs;      /**< Projection vectors, one row per vector and one column per channel. */
    std::string desc;         /**< Human-readable description (e.g. "PCA-v1"). */
    int kind = 0;             /**< FIFF projection item kind (FIFFV_PROJ_ITEM_*). */
    bool active = true;       /**< Whether this item is currently active. */
    bool active_file = false; /**< Whether this item was active when loaded from file. */
    bool has_meg = false;     /**< Whether the projection covers MEG channels. */
    bool has_eeg = false;     /**< Whether the projection covers EEG channels. */
};

} // NAMESPACE mne_mne

#endif // MNEPROJITEM_H
