//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     fwd_thread_arg.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     March 2017
 * @brief    Per-thread work packet (dipole range, coil set, output column) consumed by the parallel forward-solution dipole loop.
 *
 * Building the lead-field matrix G is embarrassingly parallel along the
 * source dimension: each dipole's column of G is independent of every
 * other dipole's column. FwdThreadArg packages everything one worker
 * thread needs to compute a slice of those columns — the source-space
 * pointer and the index range it owns, the coil set in the right
 * coordinate frame, the field/grad callback to invoke, and a write-back
 * view into the shared output matrix — so the dispatcher can hand it
 * straight to a QThreadPool::start() and recover linear speed-up on
 * multi-core hosts.
 *
 * Refactored from @c fwdThreadArgRec in MNE-C @c compute_forward.c.
 */

#ifndef FWD_THREAD_ARG_H
#define FWD_THREAD_ARG_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_global.h"
#include "fwd_types.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <functional>
#include <memory>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNELIB
{
    class MNESourceSpace;
}

//=============================================================================================================
// DEFINE NAMESPACE FWDLIB
//=============================================================================================================

namespace FWDLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class FwdCoilSet;

//=============================================================================================================
/**
 * Implements a Forward Thread Argument (replaces @c fwdThreadArg / @c fwdThreadArgRec from MNE-C @c compute_forward.c).
 *
 * @brief Per-thread work packet carrying the dipole-index range, coil set, field/grad callback and write-back view into the shared lead-field matrix — unit of work for the parallel source-space dipole loop.
 */
class FWDSHARED_EXPORT FwdThreadArg
{
public:
    typedef std::unique_ptr<FwdThreadArg> UPtr;           /**< Unique pointer type for FwdThreadArg. */

    //=========================================================================================================
    /**
     * Constructs the Forward Thread Argument
     */
    FwdThreadArg();

    //=========================================================================================================
    /**
     * Destroys the Forward Thread Argument
     */
    ~FwdThreadArg();

    //=========================================================================================================
    /**
     * Create a thread-safe duplicate for EEG parallel forward computation.
     *
     * @param[in] one         Template thread argument to duplicate.
     * @param[in] bem_model   Whether to duplicate the BEM model sub-object.
     *
     * @return Thread-safe duplicate.
     */
    static FwdThreadArg::UPtr create_eeg_multi_thread_duplicate(FwdThreadArg& one, bool bem_model);

    //=========================================================================================================
    /**
     * Create a thread-safe duplicate for MEG parallel forward computation.
     *
     * @param[in] one         Template thread argument to duplicate.
     * @param[in] bem_model   Whether to duplicate the BEM model sub-object.
     *
     * @return Thread-safe duplicate.
     */
    static FwdThreadArg::UPtr create_meg_multi_thread_duplicate(FwdThreadArg& one, bool bem_model);

public:
    Eigen::MatrixXf     *res;              /**< Destination for the solution (ncoil x nsources). */
    Eigen::MatrixXf     *res_grad;         /**< Gradient result (ncoil x 3*nsources). */
    int                 off;               /**< Offset within the result to the first source space vertex solution. */
    fwdFieldFunc        field_pot;         /**< Computes the field or potential for one dipole orientation. */
    fwdVecFieldFunc     vec_field_pot;     /**< Computes the field or potential for all dipole orientations. */
    fwdFieldGradFunc    field_pot_grad;    /**< Computes the gradient of field or potential for one dipole orientation. */
    FwdCoilSet          *coils_els;        /**< The coil definitions. */
    void                *client;           /**< Client data for the field computation function. */
    MNELIB::MNESourceSpace   *s;           /**< The source space to process. */
    bool                fixed_ori;         /**< Compute fixed orientation solution? */
    int                 comp;              /**< Which component to compute for free orientations. */
    int                 stat;              /**< Result status (OK or FAIL). */
    std::function<void()> client_free;     /**< Releases owned client sub-objects. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE FWDLIB

#endif // FWD_THREAD_ARG_H
