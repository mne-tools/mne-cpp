//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_named_vector.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Named one-dimensional counterpart of @ref mne::MNENamedMatrix.
 *
 * Stores a vector whose entries are tagged with a @c StringList of
 * labels, used for example for SSP weights or for one-sensor mappings.
 * Provides the same name-indexed access as the matrix flavour so callers
 * can robustly select sensors irrespective of their position in the
 * FIFF channel list.
 */

#ifndef MNE_NAMED_VECTOR_H
#define MNE_NAMED_VECTOR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <span>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

/**
 * Named vector - vector specification with a channel list.
 */
class MNESHARED_EXPORT MNENamedVector
{
public:
    MNENamedVector() = default;
    ~MNENamedVector() = default;

    //=========================================================================================================
    /**
     * Pick elements from this named vector by name matching, writing them
     * into a result vector ordered according to the supplied name list.
     *
     * @param[in]  pick_names   Names to pick; a name that is not present picks zero.
     * @param[in]  require_all  If true, fail when any name is not found.
     * @param[out] res          One value per name in @p pick_names.
     *
     * @return false if this vector has no names, @p res has the wrong size, or a required name is missing.
     */
    [[nodiscard]] bool pick(std::span<const std::string> pick_names, bool require_all, Eigen::Ref<Eigen::VectorXf> res) const;

    StringList names;     /**< Name list for the elements. */
    Eigen::VectorXf data; /**< The data itself. */
};

} // namespace mne

#endif // MNE_NAMED_VECTOR_H
