//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file pickresult.h
 * @since 2026
 * @date  May 2026
 * @brief Uniform pick payload (object identity, sub-index, world hit point, distance) for multimodal 3-D scenes.
 *
 * Every renderable that supports picking (BrainSurface triangles,
 * DipoleObject arrow instances, ElectrodeObject contacts,
 * SliceObject voxels) returns a @ref PickResult so the host GUI can
 * react with a single switch on @ref PickResult::kind regardless of
 * which primitive was hit.
 */

#ifndef DISP3DLIB_PICKRESULT_H
#define DISP3DLIB_PICKRESULT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QVariantMap>
#include <QVector3D>

#include <limits>

namespace DISP3DLIB
{

//=============================================================================================================
/**
 * @brief Kind of object that produced a 3-D pick.
 *
 * The enum is intentionally flat (rather than a class hierarchy) so that
 * `PickResult` stays a trivially copyable POD that can be queued, signalled,
 * and serialized without owning any GPU resources.
 */
enum class PickKind {
    None = 0,           /**< No hit. */
    CorticalVertex,      /**< Vertex on a cortical surface. */
    ElectrodeContact,    /**< Contact on an sEEG / ECoG / EEG electrode. */
    Sensor,              /**< MEG / EEG sensor element. */
    MriVoxel,            /**< Voxel on an MRI ortho slice. */
    Dipole,              /**< Dipole renderable. */
    Bem,                 /**< Triangle on a BEM surface. */
    Custom               /**< User-supplied renderable. */
};

//=============================================================================================================
/**
 * @brief Uniform payload for object-id picking across the multimodal scene.
 *
 * Producers fill in only the fields relevant to their `kind`. Consumers
 * (Pick dock, status bar, MRI ortho viewer, time-course panel) read the
 * generic fields (`kind`, `world`, `objectId`, `label`, `value`) plus any
 * kind-specific fields they understand.
 */
struct DISP3DSHARED_EXPORT PickResult
{
    /** What was hit. `None` means the ray missed every renderable. */
    PickKind    kind = PickKind::None;

    /** Hit position in world (MRI surface RAS) coordinates. */
    QVector3D   world;

    /** Stable identifier for the picked element within its renderable
     *  (e.g. cortical vertex index, contact name hash, voxel linear index).
     *  Using `qint64` lets a single field carry vertex indices, packed
     *  triplets and small string hashes. -1 means unset. */
    qint64      objectId = -1;

    /** Human-readable label (contact name, vertex coordinate string, …). */
    QString     label;

    /** Optional scalar overlay value at the picked element (uV, dSPM, MRI
     *  intensity). NaN means "not provided". */
    float       value = std::numeric_limits<float>::quiet_NaN();

    /** Source renderable identifier — opaque string assigned by the
     *  MultimodalScene when the renderable was added. Lets consumers
     *  route the pick back to the originating layer (e.g. "cortex_lh",
     *  "mri_axial", "seeg_LH"). */
    QString     sourceId;

    //--------------------------------------------------------------------
    // Kind-specific fields. Producers fill in only what applies.
    //--------------------------------------------------------------------

    /** Hemisphere for `CorticalVertex` (0 = left, 1 = right). */
    int         hemisphere = -1;

    /** Voxel coordinates for `MriVoxel`. */
    QVector3D   voxel;

    /** MRI slice orientation index for `MriVoxel`
     *  (0 = axial, 1 = sagittal, 2 = coronal). */
    int         sliceOrientation = -1;

    /** Time index for time-varying overlays (`-1` = not time-resolved). */
    int         timeSample = -1;

    /** Catch-all for renderable-specific extras (shaft name, atlas region,
     *  dipole amplitude vector, …). Kept as `QVariantMap` rather than
     *  `QJsonObject` so producers can ship `QVector3D` / `Eigen` types
     *  without going through JSON. */
    QVariantMap extras;
};

//=============================================================================================================
/**
 * @return True iff the pick reports a hit (`kind != None`).
 */
inline bool isHit(const PickResult& r) noexcept
{
    return r.kind != PickKind::None;
}

} // namespace DISP3DLIB

#endif // DISP3DLIB_PICKRESULT_H
