//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file filter_thread_arg.h
 * @since March 2026
 * @brief Argument record passed to a background raw-data filter worker.
 *
 * @ref MNELIB::FilterThreadArg bundles the input buffer, an
 * @ref MNELIB::MNEFilterDef and the completion callback that the
 * legacy raw-browser filter worker thread needs to run a filter on one
 * FIFF data buffer asynchronously. Kept POD-like so it can be passed
 * across @c QThread boundaries without ownership headaches.
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
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QWeakPointer>
#include <QTextStream>

#include <memory>

namespace FIFFLIB { class FiffCoordTrans; }

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
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
    typedef QSharedPointer<FilterThreadArg> SPtr;              /**< Shared pointer type for FilterThreadArg. */
    typedef QSharedPointer<const FilterThreadArg> ConstSPtr;   /**< Const shared pointer type for FilterThreadArg. */

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
    MNESourceSpace* s;           /**< The source space to process. */
    std::unique_ptr<FIFFLIB::FiffCoordTrans> mri_head_t;  /**< MRI-to-head coordinate transformation. */
    QWeakPointer<MNESurface> surf;  /**< Non-owning reference to the inner skull BEM surface (caller holds QSharedPointer). */
    float          limit;           /**< Distance limit for filtering (meters). */
    QTextStream    *filtered;       /**< Optional stream to log omitted point locations (may be NULL). */
    int            stat;            /**< Status code indicating whether filtering succeeded. */

// ### OLD STRUCT ###
//typedef struct {
//    MNESourceSpace* s;           /* The source space to process */
//    FiffCoordTransOld* mri_head_t;  /* Coordinate transformation */
//    MNESurface*   surf;          /* The inner skull surface */
//    float          limit;           /* Distance limit */
//    FILE           *filtered;       /* Log omitted point locations here */
//    int            stat;            /* How was it? */
//} *filterThreadArg,filterThreadArgRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // FILTERTHREADARG_H
