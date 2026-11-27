//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     pickresult.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Uniform pick payload (object identity, sub-index, world hit point, distance) for multimodal 3-D scenes.
 *
 * Every renderable that supports picking (BrainSurface triangles,
 * DipoleObject arrow instances, ElectrodeObject contacts,
 * SliceObject voxels) returns a @ref mne::viz::PickResult so the host GUI can
 * react with a single switch on @ref mne::viz::PickResult::kind "PickResult::kind" regardless of
 * which primitive was hit.
 */

#ifndef VIZ_PICKRESULT_H
#define VIZ_PICKRESULT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <limits>
#include <map>
#include <string>

namespace mne::viz
{

//=============================================================================================================
/**
 * @brief Kind of object that produced a 3-D pick.
 *
 * The enum is intentionally flat (rather than a class hierarchy) so that
 * `PickResult` stays a plain value type that can be queued, signalled,
 * and serialized without owning any GPU resources.
 */
enum class PickKind
{
    None = 0,         /**< No hit. */
    CorticalVertex,   /**< Vertex on a cortical surface. */
    ElectrodeContact, /**< Contact on an sEEG / ECoG / EEG electrode. */
    Sensor,           /**< MEG / EEG sensor element. */
    MriVoxel,         /**< Voxel on an MRI ortho slice. */
    Dipole,           /**< Dipole renderable. */
    Bem,              /**< Triangle on a BEM surface. */
    Custom            /**< User-supplied renderable. */
};

//=============================================================================================================
/**
 * @brief Uniform payload for object-id picking across the multimodal scene.
 *
 * Producers fill in only the fields relevant to their `kind`. Consumers
 * (Pick dock, status bar, MRI ortho viewer, time-course panel) read the
 * generic fields (`kind`, `world`, `objectId`, `label`, `value`) plus any
 * kind-specific fields they understand.
 *
 * @snippet ex_disp3d_scene/main.cpp pick_result_usage
 */
struct VIZSHARED_EXPORT PickResult
{
    /** What was hit. `None` means the ray missed every renderable. */
    PickKind kind = PickKind::None;

    /** Hit position in world (MRI surface RAS) coordinates. */
    Eigen::Vector3f world = Eigen::Vector3f::Zero();

    /** Stable identifier for the picked element within its renderable
     *  (e.g. cortical vertex index, contact name hash, voxel linear index).
     *  Using `std::int64_t` lets a single field carry vertex indices, packed
     *  triplets and small string hashes. -1 means unset. */
    std::int64_t objectId = -1;

    /** Human-readable label (contact name, vertex coordinate string, …). */
    std::string label;

    /** Optional scalar overlay value at the picked element (uV, dSPM, MRI
     *  intensity). NaN means "not provided". */
    float value = std::numeric_limits<float>::quiet_NaN();

    /** Source renderable identifier — opaque string assigned by the
     *  MultimodalScene when the renderable was added. Lets consumers
     *  route the pick back to the originating layer (e.g. "cortex_lh",
     *  "mri_axial", "seeg_LH"). */
    std::string sourceId;

    //--------------------------------------------------------------------
    // Kind-specific fields. Producers fill in only what applies.
    //--------------------------------------------------------------------

    /** Hemisphere for `CorticalVertex` (0 = left, 1 = right). */
    int hemisphere = -1;

    /** Voxel coordinates for `MriVoxel`. */
    Eigen::Vector3f voxel = Eigen::Vector3f::Zero();

    /** MRI slice orientation index for `MriVoxel`
     *  (0 = axial, 1 = coronal, 2 = sagittal). */
    int sliceOrientation = -1;

    /** Time index for time-varying overlays (`-1` = not time-resolved). */
    int timeSample = -1;

    /** Catch-all for renderable-specific extras as text by key (shaft name,
     *  atlas region, dipole amplitude, …). */
    std::map<std::string, std::string> extras;
};

//=============================================================================================================
/**
 * @param[in] r   Pick result to test.
 * @return True iff the pick reports a hit (`kind != None`).
 */
inline bool isHit(const PickResult& r) noexcept
{
    return r.kind != PickKind::None;
}

} // namespace mne::viz

#endif // VIZ_PICKRESULT_H
