//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file raypicker.h
 * @since March 2026
 * @brief Mouse-ray vs scene-object intersection for picking dipoles, electrodes and surfaces.
 *
 * RayPicker unprojects the current mouse position through the
 * active camera into a world-space ray and tests it against every
 * selectable scene primitive: per-triangle Moller-Trumbore against
 * @ref BrainSurface meshes, per-arrow segment / cylinder for
 * @ref DipoleObject and ECoG electrodes, and per-sphere for
 * source-space points and digitizer fiducials.
 *
 * The closest hit is returned as a @ref PickResult that names the
 * object, the triangle / instance index and the world-space
 * intersection point &mdash; enough for the surrounding GUI to
 * highlight a region, show a tooltip, or seed an interactive label.
 */

#ifndef RAYPICKER_H
#define RAYPICKER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

#include "core/viewstate.h"

#include <QPoint>
#include <QRect>
#include <QMatrix4x4>
#include <QVector3D>
#include <QMap>
#include <QStandardItem>

#include <memory>

class BrainSurface;
class DipoleObject;

//=============================================================================================================
/**
 * @brief Result of a ray–mesh intersection test containing the hit point, triangle index, and distance.
 *
 * Describes the result of a ray-pick operation.
 */
struct DISP3DSHARED_EXPORT PickResult
{
    bool    hit             = false;     ///< True if something was hit
    float   distance        = 0.0f;     ///< Distance along ray to hit point
    QVector3D hitPoint;                 ///< World-space intersection point

    QStandardItem *item     = nullptr;  ///< Tree item that was hit (nullable)
    QString surfaceKey;                 ///< FsSurface map key of the hit surface
    int     vertexIndex     = -1;       ///< Vertex or element index at hit

    bool    isDipole        = false;    ///< True if a dipole was hit
    int     dipoleIndex     = -1;       ///< Index within the dipole set

    // FsAnnotation info (brain surfaces only)
    QString regionName;                 ///< FsAnnotation region label (if available)
    int     regionId        = -1;       ///< FsAnnotation label ID

    //=========================================================================================================
    /**
     * Build a human-readable display label for the hit object.
     * Includes annotation region, sensor name, BEM compartment, etc.
     *
     * @return Display label string, empty if no hit.
     */
    QString displayLabel() const;
};

//=============================================================================================================
/**
 * Stateless ray-picking utility.
 *
 * Given camera matrices, a screen position, and the scene contents, this
 * class casts a ray and returns a PickResult describing the closest
 * intersection.  It has no state of its own — hover tracking is handled
 * by the caller.
 *
 * @brief    Ray casting and intersection testing.
 */
class DISP3DSHARED_EXPORT RayPicker
{
public:
    //=========================================================================================================
    /**
     * Unproject a screen position to a world-space ray.
     *
     * @param[in]  screenPos    2D cursor position (widget coords).
     * @param[in]  paneRect     Rectangle of the active viewport pane.
     * @param[in]  pvm          Combined projection × view × model matrix.
     * @param[out] rayOrigin    Ray origin (world space).
     * @param[out] rayDir       Ray direction (normalised, world space).
     * @return                  True if the unproject succeeded.
     */
    static bool unproject(const QPoint &screenPos,
                          const QRect &paneRect,
                          const QMatrix4x4 &pvm,
                          QVector3D &rayOrigin,
                          QVector3D &rayDir);

    //=========================================================================================================
    /**
     * Cast a ray against all surfaces in the scene.
     *
     * @param[in] rayOrigin       World-space ray origin.
     * @param[in] rayDir          World-space ray direction (normalised).
     * @param[in] subView         Active SubView (for visibility / surface type filtering).
     * @param[in] surfaces        Map of surface key → BrainSurface.
     * @param[in] itemSurfaceMap  Map of tree item → BrainSurface (for reverse lookup).
     * @param[in] itemDipoleMap   Map of tree item → DipoleObject.
     * @return                    PickResult for the closest hit.
     */
    static PickResult pick(const QVector3D &rayOrigin,
                           const QVector3D &rayDir,
                           const SubView &subView,
                           const QMap<QString, std::shared_ptr<BrainSurface>> &surfaces,
                           const QMap<const QStandardItem*, std::shared_ptr<BrainSurface>> &itemSurfaceMap,
                           const QMap<const QStandardItem*, std::shared_ptr<DipoleObject>> &itemDipoleMap);

    //=========================================================================================================
    /**
     * Build a display label from a PickResult.
     *
     * This examines the surface key, annotation data, and item type
     * to produce a human-readable hover label.
     *
     * @param[in] result          PickResult from pick().
     * @param[in] itemSurfaceMap  Map for reverse item → surface lookup.
     * @param[in] surfaces        FsSurface map for key-based lookup.
     * @return                    Display label string.
     */
    static QString buildLabel(const PickResult &result,
                              const QMap<const QStandardItem*, std::shared_ptr<BrainSurface>> &itemSurfaceMap,
                              const QMap<QString, std::shared_ptr<BrainSurface>> &surfaces);
};

#endif // RAYPICKER_H
