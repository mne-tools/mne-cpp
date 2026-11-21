//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     filter_thread_arg.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     January 2017
 * @brief    Argument record passed to a background raw-data filter worker.
 *
 * @ref mne::FilterThreadArg bundles the input buffer, an
 * @ref mne::MNEFilterDef and the completion callback that the
 * legacy raw-browser filter worker thread needs to run a filter on one
 * FIFF data buffer asynchronously. Kept POD-like so it can be passed
 * across thread boundaries without ownership headaches.
 */

#ifndef FILTERTHREADARG_H
#define FILTERTHREADARG_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_source_space.h"
#include "mne_surface.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <ostream>

namespace mne
{
class FiffCoordTrans;
}

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Implements a Filter Thread Argument (Replaces *filterThreadArg,filterThreadArgRec; struct of MNE-C filter_source_space.c).
 *
 * @brief Thread-local arguments for parallel raw data filtering (channel range, filter kernel, buffer).
 */
class MNESHARED_EXPORT FilterThreadArg
{
public:
    //=========================================================================================================
    /**
     * Constructs the Filter Thread Argument
     * Refactored: new_filter_thread_arg (filter_source_space.c)
     */
    FilterThreadArg();

    //=========================================================================================================
    /**
     * Destroys the Filter Thread Argument
     * Refactored: free_filter_thread_arg (filter_source_space.c)
     */
    ~FilterThreadArg();

public:
    MNESourceSpace* s;                               /**< The source space to process. */
    std::unique_ptr<mne::FiffCoordTrans> mri_head_t; /**< MRI-to-head coordinate transformation. */
    std::weak_ptr<MNESurface> surf;                  /**< Non-owning reference to the inner skull BEM surface (owned by the caller). */
    float limit;                                     /**< Distance limit for filtering (meters). */
    std::ostream* filtered;                          /**< Optional stream to log omitted point locations (may be NULL). */
    int stat;                                        /**< Status code indicating whether filtering succeeded. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE mne_mne

#endif // FILTERTHREADARG_H
