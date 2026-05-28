//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file fwd_field_map.h
 * @since 2026
 * @date  March 2026
 * @brief Sphere-model field interpolator that maps measured MEG/EEG values onto a dense scalp or cortical surface.
 *
 * Field mapping is the dual of source estimation: instead of recovering
 * cortical currents, it produces a smooth continuous field on a target
 * surface that exactly reproduces the recorded sensor values. The
 * classical MNE approach (Hamalainen 1994; Ahlfors et al. 2010) treats
 * every sensor as a dipole layer on a sphere fitted to the head and
 * evaluates the Legendre-series lead-field dot products
 * @c ⟨L_i, L_j⟩ between sensor pairs (@c self_dots) and between sensors
 * and surface vertices (@c surface_dots). Solving the regularised system
 * @c (Gᵢⱼ + λI) x = b yields per-vertex interpolation weights.
 *
 * FwdFieldMap implements that recipe for the spherical-model special
 * case: fast @c _fast_sphere_dot_r0 lead-field dots, an SVD-based
 * pseudo-inverse with eigenvalue truncation (@c miss) for regularisation,
 * and optional pre-projection through the SSP operator to keep the map
 * orthogonal to projected-out noise subspaces. The output matrix is
 * channel-to-vertex and is reused across every sample of a recording.
 *
 * Ported from MNE-Python (BSD-3-Clause):
 *   @c _do_self_dots, @c _do_surface_dots, @c _fast_sphere_dot_r0
 *   (@c mne/forward/_lead_dots.py);
 *   @c _compute_mapping_matrix, @c _pinv_trunc
 *   (@c mne/forward/_field_interpolation.py).
 */

#ifndef FWD_FIELD_MAP_H
#define FWD_FIELD_MAP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_global.h"

#include <Eigen/Core>
#include <memory>
#include "fwd_coil_set.h"
#include <fiff/fiff_info.h>

//=============================================================================================================
// DEFINE NAMESPACE FWDLIB
//=============================================================================================================

namespace FWDLIB
{

//=============================================================================================================
/**
 * @brief Computes the per-vertex sensor-to-surface mapping matrix used to render a continuous scalp/cortex field that exactly reproduces the recorded MEG/EEG values.
 *
 * Uses Legendre-series lead-field dot products evaluated on a sphere
 * fitted to the head together with an SVD-based pseudo-inverse with
 * eigenvalue truncation — the classic MNE field-interpolation recipe.
 *
 * Ported from MNE-Python (BSD-3-Clause):
 *   @c _do_self_dots, @c _do_surface_dots, @c _fast_sphere_dot_r0
 *   (@c _lead_dots.py);
 *   @c _compute_mapping_matrix, @c _pinv_trunc
 *   (@c _field_interpolation.py).
 */
class FWDSHARED_EXPORT FwdFieldMap
{
public:
    /**
     * Compute MEG sensor-to-surface mapping matrix.
     *
     * @param[in] coils     MEG coils (already transformed to target coordinate frame).
     * @param[in] vertices  FsSurface vertex positions (nvert × 3).
     * @param[in] normals   FsSurface vertex normals (nvert × 3).
     * @param[in] origin    Sphere origin in the target coordinate frame.
     * @param[in] intrad    Integration radius (default 0.06, matching MNE-Python _setup_dots).
     * @param[in] miss      Eigenvalue truncation threshold (default 1e-4, matching _make_surface_mapping).
     * @return Mapping matrix (nvert × nchan), or null on failure.
     */
    static std::unique_ptr<Eigen::MatrixXf> computeMegMapping(const FwdCoilSet& coils,
                                                              const Eigen::MatrixX3f& vertices,
                                                              const Eigen::MatrixX3f& normals,
                                                              const Eigen::Vector3f& origin,
                                                              float intrad = 0.06f,
                                                              float miss = 1e-4f);

    /**
     * Compute MEG sensor-to-surface mapping matrix with SSP projection.
     *
     * @param[in] coils     MEG coils (already transformed to target coordinate frame).
     * @param[in] vertices  FsSurface vertex positions (nvert × 3).
     * @param[in] normals   FsSurface vertex normals (nvert × 3).
     * @param[in] origin    Sphere origin in the target coordinate frame.
     * @param[in] info      FiffInfo with SSP projectors and channel names.
     * @param[in] chNames   Channel names corresponding to the coils (for projector matching).
     * @param[in] intrad    Integration radius (default 0.06).
     * @param[in] miss      Eigenvalue truncation threshold (default 1e-4).
     * @return Mapping matrix (nvert × nchan), or null on failure.
     */
    static std::unique_ptr<Eigen::MatrixXf> computeMegMapping(const FwdCoilSet& coils,
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
     * @param[in] vertices  FsSurface vertex positions (nvert × 3).
     * @param[in] origin    Sphere origin in the target coordinate frame.
     * @param[in] intrad    Base integration radius (default 0.06, matching MNE-Python _setup_dots).
     * @param[in] miss      Eigenvalue truncation threshold (default 1e-3, matching _make_surface_mapping).
     * @return Mapping matrix (nvert × nchan), or null on failure.
     */
    static std::unique_ptr<Eigen::MatrixXf> computeEegMapping(const FwdCoilSet& coils,
                                                              const Eigen::MatrixX3f& vertices,
                                                              const Eigen::Vector3f& origin,
                                                              float intrad = 0.06f,
                                                              float miss = 1e-3f);

    /**
     * Compute EEG electrode-to-surface mapping matrix with SSP projection
     * and average reference.
     *
     * @param[in] coils     EEG electrodes (already transformed to target coordinate frame).
     * @param[in] vertices  FsSurface vertex positions (nvert × 3).
     * @param[in] origin    Sphere origin in the target coordinate frame.
     * @param[in] info      FiffInfo with SSP projectors and channel names.
     * @param[in] chNames   Channel names corresponding to the coils (for projector matching).
     * @param[in] intrad    Base integration radius (default 0.06).
     * @param[in] miss      Eigenvalue truncation threshold (default 1e-3).
     * @return Mapping matrix (nvert × nchan), or null on failure.
     */
    static std::unique_ptr<Eigen::MatrixXf> computeEegMapping(const FwdCoilSet& coils,
                                                              const Eigen::MatrixX3f& vertices,
                                                              const Eigen::Vector3f& origin,
                                                              const FIFFLIB::FiffInfo& info,
                                                              const QStringList& chNames,
                                                              float intrad = 0.06f,
                                                              float miss = 1e-3f);
};

//=============================================================================================================
} // NAMESPACE FWDLIB

#endif // FWD_FIELD_MAP_H
