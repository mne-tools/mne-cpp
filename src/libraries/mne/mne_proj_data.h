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
 * @ref MNELIB::MNEProjData ports @c mneProjDataRec - the cached state
 * kept by the C raw-data path so projection vectors do not have to be
 * rebuilt for every buffer. It owns the active @ref MNEProjOp, the
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
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
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
    typedef QSharedPointer<MNEProjData> SPtr;              /**< Shared pointer type for MNEProjData. */
    typedef QSharedPointer<const MNEProjData> ConstSPtr;   /**< Const shared pointer type for MNEProjData. */

    //=========================================================================================================
    /**
     * Constructs the MNEProjData.
     */
    MNEProjData(const MNELIB::MNESurface* s);

    //=========================================================================================================
    /**
     * Destroys the MNEProjData.
     */
    ~MNEProjData() = default;

public:
    Eigen::VectorXf a;      /**< Triangle-local dot product r12 . r12 for each triangle. */
    Eigen::VectorXf b;      /**< Triangle-local dot product r13 . r13 for each triangle. */
    Eigen::VectorXf c;      /**< Triangle-local dot product r12 . r13 for each triangle. */
    Eigen::VectorXi act;    /**< Per-triangle boolean flag: 1 if this triangle is active. */
    int   nactive = 0;      /**< Number of currently active triangles. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEPROJDATA_H
