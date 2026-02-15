//=============================================================================================================
/**
 * @file     raypicker.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
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
 * @brief    RayPicker class declaration — ray casting and intersection testing.
 *
 */

#ifndef RAYPICKER_H
#define RAYPICKER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_rhi_global.h"

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
 * Describes the result of a ray-pick operation.
 */
struct PickResult
{
    bool    hit             = false;     ///< True if something was hit
    float   distance        = 0.0f;     ///< Distance along ray to hit point
    QVector3D hitPoint;                 ///< World-space intersection point

    QStandardItem *item     = nullptr;  ///< Tree item that was hit (nullable)
    QString surfaceKey;                 ///< Surface map key of the hit surface
    int     vertexIndex     = -1;       ///< Vertex or element index at hit

    bool    isDipole        = false;    ///< True if a dipole was hit
    int     dipoleIndex     = -1;       ///< Index within the dipole set

    // Annotation info (brain surfaces only)
    QString regionName;                 ///< Annotation region label (if available)
    int     regionId        = -1;       ///< Annotation label ID

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
class DISP3DRHISHARED_EXPORT RayPicker
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
     * @param[in] surfaces        Surface map for key-based lookup.
     * @return                    Display label string.
     */
    static QString buildLabel(const PickResult &result,
                              const QMap<const QStandardItem*, std::shared_ptr<BrainSurface>> &itemSurfaceMap,
                              const QMap<QString, std::shared_ptr<BrainSurface>> &surfaces);
};

#endif // RAYPICKER_H
