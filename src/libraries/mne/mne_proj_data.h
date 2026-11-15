//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_proj_data.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Legacy MNE-C aggregator for SSP projections plus the channel list they apply to.
 *
 * @ref mne::MNEProjData ports @c mneProjDataRec - the cached state
 * kept by the C raw-data path so projection vectors do not have to be
 * rebuilt for every buffer. It owns the active @ref mne::MNEProjOp, the
 * channel-name list it was assembled against and the dirty flag the C
 * tools used to invalidate the cache when the channel selection changed.
 */

#ifndef MNEPROJDATA_H
#define MNEPROJDATA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class MNESurface;

//=============================================================================================================
/**
 * Replaces *projData,projDataRec struct (mne_project_to_surface.c).
 *
 * @brief Auxiliary projection data computed from MNEProjOp for efficient repeated application.
 */
class MNESHARED_EXPORT MNEProjData
{
public:
    //=========================================================================================================
    /**
     * Constructs the MNEProjData.
     *
     * @param[in] s  Surface whose triangles are precomputed (non-owning, must not be null); all triangles start active.
     */
    MNEProjData(const mne::MNESurface* s);

    //=========================================================================================================
    /**
     * Destroys the MNEProjData.
     */
    ~MNEProjData() = default;

public:
    Eigen::VectorXf a;   /**< Triangle-local dot product r12 . r12 for each triangle. */
    Eigen::VectorXf b;   /**< Triangle-local dot product r13 . r13 for each triangle. */
    Eigen::VectorXf c;   /**< Triangle-local dot product r12 . r13 for each triangle. */
    Eigen::VectorXi act; /**< Per-triangle boolean flag: 1 if this triangle is active. */
    int nactive = 0;     /**< Number of currently active triangles. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE mne_mne

#endif // MNEPROJDATA_H
