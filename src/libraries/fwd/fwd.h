//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fwd.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     December 2012
 * @brief    Thin static-wrapper façade exposing the MNE Matlab-toolbox forward-solution entry points (e.g. mne_read_forward_solution).
 *
 * The MNE Matlab toolbox defines a flat set of top-level functions
 * (@c mne_read_forward_solution, @c mne_pick_channels_forward, ...) that
 * users have long scripted against. Fwd re-exposes the C++ equivalents
 * implemented on @c MNELIB::MNEForwardSolution as @c static inline
 * methods with matching names, signatures and side effects, so a port
 * of an existing Matlab pipeline can be performed call-for-call without
 * having to thread an object instance through every helper.
 */

#ifndef FWD_H
#define FWD_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_global.h"
#include <mne/mne_forward_solution.h>

#include <fiff/fiff_constants.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QIODevice>
#include <QStringList>

//=============================================================================================================
// DEFINE NAMESPACE FWDLIB
//=============================================================================================================

namespace FWDLIB
{

//=============================================================================================================
/**
 * @brief Static-method façade that re-exposes MNEForwardSolution toolbox-equivalent entry points under their familiar @c mne_* names.
 */
class FWDSHARED_EXPORT Fwd
{
public:

    //=========================================================================================================
    /**
     * Destructor.
     */
    virtual ~Fwd()
    { }

    //=========================================================================================================
    /**
     * mne_read_forward_solution
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the MNEForwardSolution::read static function
     *
     * Reads a forward solution from a fif file
     *
     * @param[in] p_IODevice    A fiff IO device like a fiff QFile or QTCPSocket.
     * @param[in, out] fwd      A forward solution from a fif file.
     * @param[in] force_fixed   Force fixed source orientation mode? (optional).
     * @param[in] surf_ori      Use surface based source coordinate system? (optional).
     * @param[in] include       Include these channels (optional).
     * @param[in] exclude       Exclude these channels (optional).
     *
     * @return true if succeeded, false otherwise.
     */
    static inline bool read_forward_solution(QIODevice& p_IODevice,
                                             MNELIB::MNEForwardSolution& fwd,
                                             bool force_fixed = false,
                                             bool surf_ori = false,
                                             const QStringList& include = FIFFLIB::defaultQStringList,
                                             const QStringList& exclude = FIFFLIB::defaultQStringList)
    {
        return MNELIB::MNEForwardSolution::read(p_IODevice,
                                        fwd,
                                        force_fixed,
                                        surf_ori,
                                        include,
                                        exclude);
    }
};

} // NAMESPACE FWDLIB

#endif // FWD_H
