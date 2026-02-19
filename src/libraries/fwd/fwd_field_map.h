//=============================================================================================================
/**
 * @file     fwd_field_map.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     January, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Sphere-model field mapping.
 *
 * C++ port of MNE-Python field mapping (BSD-3-Clause, The MNE-Python contributors).
 * Source files: mne/forward/_lead_dots.py, mne/forward/_field_interpolation.py
 *
 */

#ifndef FWD_FIELD_MAP_H
#define FWD_FIELD_MAP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_global.h"

#include <Eigen/Core>
#include <QSharedPointer>
#include "fwd_coil_set.h"
#include <fiff/fiff_info.h>

//=============================================================================================================
// DEFINE NAMESPACE FWDLIB
//=============================================================================================================

namespace FWDLIB
{

//=============================================================================================================
/**
 * Sphere-model based sensor-to-surface field mapping.
 *
 * Uses Legendre polynomial series for lead-field dot products and SVD-based
 * pseudo-inverse with eigenvalue truncation, matching the classic MNE field
 * interpolation approach.
 *
 * Ported from MNE-Python (BSD-3-Clause):
 *   _do_self_dots, _do_surface_dots, _fast_sphere_dot_r0  (_lead_dots.py)
 *   _compute_mapping_matrix, _pinv_trunc  (_field_interpolation.py)
 */
class FWDSHARED_EXPORT FwdFieldMap
{
public:
    /**
     * Compute MEG sensor-to-surface mapping matrix.
     *
     * @param[in] coils     MEG coils (already transformed to target coordinate frame).
     * @param[in] vertices  Surface vertex positions (nvert × 3).
     * @param[in] normals   Surface vertex normals (nvert × 3).
     * @param[in] origin    Sphere origin in the target coordinate frame.
     * @param[in] intrad    Integration radius (default 0.06, matching MNE-Python _setup_dots).
     * @param[in] miss      Eigenvalue truncation threshold (default 1e-4, matching _make_surface_mapping).
     * @return Mapping matrix (nvert × nchan), or null on failure.
     */
    static QSharedPointer<Eigen::MatrixXf> computeMegMapping(const FwdCoilSet& coils,
                                                             const Eigen::MatrixX3f& vertices,
                                                             const Eigen::MatrixX3f& normals,
                                                             const Eigen::Vector3f& origin,
                                                             float intrad = 0.06f,
                                                             float miss = 1e-4f);

    /**
     * Compute MEG sensor-to-surface mapping matrix with SSP projection.
     *
     * @param[in] coils     MEG coils (already transformed to target coordinate frame).
     * @param[in] vertices  Surface vertex positions (nvert × 3).
     * @param[in] normals   Surface vertex normals (nvert × 3).
     * @param[in] origin    Sphere origin in the target coordinate frame.
     * @param[in] info      FiffInfo with SSP projectors and channel names.
     * @param[in] chNames   Channel names corresponding to the coils (for projector matching).
     * @param[in] intrad    Integration radius (default 0.06).
     * @param[in] miss      Eigenvalue truncation threshold (default 1e-4).
     * @return Mapping matrix (nvert × nchan), or null on failure.
     */
    static QSharedPointer<Eigen::MatrixXf> computeMegMapping(const FwdCoilSet& coils,
                                                             const Eigen::MatrixX3f& vertices,
                                                             const Eigen::MatrixX3f& normals,
                                                             const Eigen::Vector3f& origin,
                                                             const FIFFLIB::FiffInfo& info,
                                                             const QStringList& chNames,
                                                             float intrad = 0.06f,
                                                             float miss = 1e-4f);

    /**
     * Compute EEG electrode-to-surface mapping matrix.
     *
     * The effective integration radius is intrad * 0.7, matching MNE-Python's
     * internal scaling in _do_self_dots / _do_surface_dots for EEG.
     *
     * @param[in] coils     EEG electrodes (already transformed to target coordinate frame).
     * @param[in] vertices  Surface vertex positions (nvert × 3).
     * @param[in] origin    Sphere origin in the target coordinate frame.
     * @param[in] intrad    Base integration radius (default 0.06, matching MNE-Python _setup_dots).
     * @param[in] miss      Eigenvalue truncation threshold (default 1e-3, matching _make_surface_mapping).
     * @return Mapping matrix (nvert × nchan), or null on failure.
     */
    static QSharedPointer<Eigen::MatrixXf> computeEegMapping(const FwdCoilSet& coils,
                                                             const Eigen::MatrixX3f& vertices,
                                                             const Eigen::Vector3f& origin,
                                                             float intrad = 0.06f,
                                                             float miss = 1e-3f);

    /**
     * Compute EEG electrode-to-surface mapping matrix with SSP projection
     * and average reference.
     *
     * @param[in] coils     EEG electrodes (already transformed to target coordinate frame).
     * @param[in] vertices  Surface vertex positions (nvert × 3).
     * @param[in] origin    Sphere origin in the target coordinate frame.
     * @param[in] info      FiffInfo with SSP projectors and channel names.
     * @param[in] chNames   Channel names corresponding to the coils (for projector matching).
     * @param[in] intrad    Base integration radius (default 0.06).
     * @param[in] miss      Eigenvalue truncation threshold (default 1e-3).
     * @return Mapping matrix (nvert × nchan), or null on failure.
     */
    static QSharedPointer<Eigen::MatrixXf> computeEegMapping(const FwdCoilSet& coils,
                                                             const Eigen::MatrixX3f& vertices,
                                                             const Eigen::Vector3f& origin,
                                                             const FIFFLIB::FiffInfo& info,
                                                             const QStringList& chNames,
                                                             float intrad = 0.06f,
                                                             float miss = 1e-3f);
};

//=============================================================================================================
// BACKWARD COMPATIBILITY – keep old name available
//=============================================================================================================

/** @deprecated Use FwdFieldMap instead. */
using FieldMap = FwdFieldMap;

//=============================================================================================================
} // NAMESPACE FWDLIB

#endif // FWD_FIELD_MAP_H
