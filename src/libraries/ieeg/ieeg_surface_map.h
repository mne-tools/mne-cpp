//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     ieeg_surface_map.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Map sEEG / ECoG contact values onto surface vertices, as MNE-Python's stc_near_sensors.
 *
 * Each vertex within @c distance of a contact takes on that contact's value, weighted by
 * 1 - d / distance, so the weight is 1 at the contact and falls linearly to 0 at @c distance.
 * The result is a sparse vertices x contacts matrix W, and vertex data = W * contact data.
 */

#ifndef IEEG_SURFACE_MAP_H
#define IEEG_SURFACE_MAP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ieeg_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// DEFINE NAMESPACE IEEGLIB
//=============================================================================================================

namespace IEEGLIB
{

//=============================================================================================================
/**
 * @brief How the weights of contacts near one vertex combine; the modes of MNE-Python's stc_near_sensors.
 */
enum class IeegSurfaceMapMode
{
    Sum,     /**< Sum of the weights of all contacts within reach. */
    Single,  /**< Weight of the nearest contact only. */
    Nearest, /**< Value of the nearest contact within reach, unweighted. */
    Weighted /**< As Sum, with the weights of each vertex normalised to sum to 1. */
};

//=============================================================================================================
/**
 * @brief Weights that paint contact values onto a surface.
 *
 * @snippet ex_ieeg/main.cpp ieeg_surface_map_usage
 */
class IEEGSHARED_EXPORT IeegSurfaceMap
{
public:
    //=========================================================================================================
    /**
     * Compute the vertices x contacts weight matrix.
     *
     * Equivalent to MNE-Python's `stc_near_sensors` for a surface. Contacts with a non-finite
     * position get an empty column.
     *
     * @param[in] contacts  Contact positions, one row per contact, in metres.
     * @param[in] vertices  Surface vertex positions in the same frame, one row per vertex.
     * @param[in] distance  Reach of each contact in metres.
     * @param[in] mode      How weights near one vertex combine.
     * @param[in] project   Move each contact to its nearest vertex first, as ECoG contacts
     *                      lie on, not in, the cortex.
     * @return Sparse vertices x contacts weights.
     */
    static Eigen::SparseMatrix<double> weights(const Eigen::MatrixX3f& contacts,
                                               const Eigen::MatrixX3f& vertices,
                                               double distance = 0.01,
                                               IeegSurfaceMapMode mode = IeegSurfaceMapMode::Sum,
                                               bool project = true);
};

} // namespace IEEGLIB

#endif // IEEG_SURFACE_MAP_H
