//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_named_vector.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Named one-dimensional counterpart of @ref MNELIB::MNENamedMatrix.
 *
 * Stores a vector whose entries are tagged with a @c QStringList of
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

#include <Eigen/Core>

#include <QStringList>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
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
     * @param[in]  names        List of names to pick.
     * @param[in]  nnames       Number of names in the list.
     * @param[in]  require_all  If true, fail when any name is not found.
     * @param[out] res          Output vector (must have at least nnames elements).
     *
     * @return OK on success, FAIL on error.
     */
    int pick(const QStringList& names, int nnames, bool require_all, Eigen::Ref<Eigen::VectorXf> res) const;

    int         nvec = 0;   /**< Number of elements. */
    QStringList names;      /**< Name list for the elements. */
    Eigen::VectorXf data;      /**< The data itself. */
};

} // namespace MNELIB

#endif // MNE_NAMED_VECTOR_H
