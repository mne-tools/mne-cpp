//=============================================================================================================
/**
 * @file     raypicker.cpp
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
 * @brief    RayPicker class implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "raypicker.h"
#include "renderable/brainsurface.h"
#include "renderable/dipoleobject.h"
#include "model/items/abstracttreeitem.h"
#include "model/items/digitizertreeitem.h"

#include <QVector4D>
#include <limits>

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

bool RayPicker::unproject(const QPoint &screenPos,
                           const QRect &paneRect,
                           const QMatrix4x4 &pvm,
                           QVector3D &rayOrigin,
                           QVector3D &rayDir)
{
    bool invertible = false;
    QMatrix4x4 invPVM = pvm.inverted(&invertible);
    if (!invertible) return false;

    const float localX = static_cast<float>(screenPos.x() - paneRect.x());
    const float localY = static_cast<float>(screenPos.y() - paneRect.y());
    const float paneW  = static_cast<float>(std::max(1, paneRect.width()));
    const float paneH  = static_cast<float>(std::max(1, paneRect.height()));

    const float ndcX = (2.0f * localX) / paneW - 1.0f;
    const float ndcY = 1.0f - (2.0f * localY) / paneH;

    QVector4D vNear(ndcX, ndcY, -1.0f, 1.0f);
    QVector4D vFar (ndcX, ndcY,  1.0f, 1.0f);

    QVector4D pNear = invPVM * vNear;
    QVector4D pFar  = invPVM * vFar;
    pNear /= pNear.w();
    pFar  /= pFar.w();

    rayOrigin = pNear.toVector3D();
    rayDir    = (pFar.toVector3D() - pNear.toVector3D()).normalized();
    return true;
}

//=============================================================================================================

PickResult RayPicker::pick(const QVector3D &rayOrigin,
                            const QVector3D &rayDir,
                            const SubView &subView,
                            const QMap<QString, std::shared_ptr<BrainSurface>> &surfaces,
                            const QMap<const QStandardItem*, std::shared_ptr<BrainSurface>> &itemSurfaceMap,
                            const QMap<const QStandardItem*, std::shared_ptr<DipoleObject>> &itemDipoleMap)
{
    PickResult result;
    float closestDist = std::numeric_limits<float>::max();

    // ── Test surfaces ──────────────────────────────────────────────────
    for (auto it = surfaces.cbegin(); it != surfaces.cend(); ++it) {
        const QString &key = it.key();
        const auto &surf   = it.value();

        if (!surf->isVisible())               continue;
        if (!subView.shouldRenderSurface(key)) continue;
        if (key.startsWith("srcsp_"))          continue;  // skip source space for picking

        const bool isSensor = key.startsWith("sens_");
        const bool isBem    = key.startsWith("bem_");
        const bool isDig    = key.startsWith("dig_");

        // Brain surfaces: only pick if matching active surface type
        if (!isSensor && !isBem && !isDig) {
            if (!subView.matchesSurfaceType(key)) continue;
        }

        float dist = 0.0f;
        int vertexIdx = -1;
        if (surf->intersects(rayOrigin, rayDir, dist, vertexIdx)) {
            if (dist < closestDist) {
                closestDist = dist;
                result.hit          = true;
                result.distance     = dist;
                result.hitPoint     = rayOrigin + dist * rayDir;
                result.vertexIndex  = vertexIdx;
                result.surfaceKey   = key;
                result.isDipole     = false;
                result.dipoleIndex  = -1;

                // Reverse lookup: find tree item for this surface
                result.item = nullptr;
                for (auto i = itemSurfaceMap.cbegin(); i != itemSurfaceMap.cend(); ++i) {
                    if (i.value() == surf) {
                        result.item = const_cast<QStandardItem*>(i.key());
                        break;
                    }
                }

                // Annotation info
                if (result.item && itemSurfaceMap.contains(result.item)) {
                    result.regionName = itemSurfaceMap[result.item]->getAnnotationLabel(vertexIdx);
                    result.regionId   = itemSurfaceMap[result.item]->getAnnotationLabelId(vertexIdx);
                } else {
                    result.regionName.clear();
                    result.regionId = -1;
                }
            }
        }
    }

    // ── Test dipoles ───────────────────────────────────────────────────
    for (auto it = itemDipoleMap.cbegin(); it != itemDipoleMap.cend(); ++it) {
        if (!subView.visibility.dipoles) continue;
        if (!it.value()->isVisible())    continue;

        float dist = 0.0f;
        int dipIdx = it.value()->intersect(rayOrigin, rayDir, dist);
        if (dipIdx != -1 && dist < closestDist) {
            closestDist = dist;
            result.hit          = true;
            result.distance     = dist;
            result.hitPoint     = rayOrigin + dist * rayDir;
            result.item         = const_cast<QStandardItem*>(it.key());
            result.surfaceKey.clear();
            result.vertexIndex  = dipIdx;
            result.isDipole     = true;
            result.dipoleIndex  = dipIdx;
            result.regionName.clear();
            result.regionId     = -1;
        }
    }

    return result;
}

//=============================================================================================================

QString RayPicker::buildLabel(const PickResult &result,
                               const QMap<const QStandardItem*, std::shared_ptr<BrainSurface>> &itemSurfaceMap,
                               const QMap<QString, std::shared_ptr<BrainSurface>> &surfaces)
{
    if (!result.hit) return QString();

    const QString &key = result.surfaceKey;

    // ── Brain surface with annotation ──────────────────────────────────
    if (!result.regionName.isEmpty()) {
        QString hemi;
        if (key.startsWith("lh")) hemi = "lh";
        else if (key.startsWith("rh")) hemi = "rh";

        return hemi.isEmpty()
            ? QString("Region: %1").arg(result.regionName)
            : QString("Region: %1 (%2)").arg(result.regionName, hemi);
    }

    // ── Dipole ─────────────────────────────────────────────────────────
    if (result.isDipole) {
        QString name = result.item ? result.item->text() : QStringLiteral("Dipole");
        return QString("%1 (Dipole %2)").arg(name).arg(result.dipoleIndex);
    }

    // ── Sensor/BEM/Digitizer/Helmet ────────────────────────────────────
    if (key.startsWith("sens_surface_meg")) {
        return QStringLiteral("MEG Helmet");
    }
    if (key.startsWith("sens_meg_")) {
        return QString("MEG: %1").arg(result.item ? result.item->text() : key);
    }
    if (key.startsWith("sens_eeg_")) {
        return QString("EEG: %1").arg(result.item ? result.item->text() : key);
    }
    if (key.startsWith("dig_")) {
        // Resolve individual point from batched mesh
        QString pointName;
        if (result.item && result.vertexIndex >= 0) {
            AbstractTreeItem *abs = dynamic_cast<AbstractTreeItem*>(result.item);
            if (abs && abs->type() == AbstractTreeItem::DigitizerItem + QStandardItem::UserType) {
                auto *digItem = static_cast<DigitizerTreeItem*>(abs);
                constexpr int vertsPerSphere = 42;
                int ptIdx = result.vertexIndex / vertsPerSphere;
                const QStringList &names = digItem->pointNames();
                if (ptIdx >= 0 && ptIdx < names.size())
                    pointName = names[ptIdx];
            }
        }
        QString category = key.mid(4);
        if (!category.isEmpty()) category[0] = category[0].toUpper();
        return pointName.isEmpty()
            ? QString("Digitizer (%1)").arg(category)
            : QString("Digitizer: %1 (%2)").arg(pointName, category);
    }
    if (key.startsWith("bem_")) {
        QString compartment = key.mid(4);
        if (!compartment.isEmpty()) compartment[0] = compartment[0].toUpper();
        compartment.replace("_", " ");
        return QString("BEM: %1").arg(compartment);
    }

    // ── Hemisphere fallback ────────────────────────────────────────────
    if (key.startsWith("lh_")) return QStringLiteral("Left Hemisphere");
    if (key.startsWith("rh_")) return QStringLiteral("Right Hemisphere");

    return QString();
}

//=============================================================================================================

QString PickResult::displayLabel() const
{
    // Delegated to the static builder in RayPicker; this method is a
    // convenience wrapper when the caller doesn't have the surface maps.
    if (!hit) return QString();

    if (!regionName.isEmpty()) {
        QString hemi;
        if (surfaceKey.startsWith("lh")) hemi = "lh";
        else if (surfaceKey.startsWith("rh")) hemi = "rh";
        return hemi.isEmpty()
            ? QString("Region: %1").arg(regionName)
            : QString("Region: %1 (%2)").arg(regionName, hemi);
    }

    if (isDipole) {
        return QString("Dipole %1").arg(dipoleIndex);
    }

    return surfaceKey;
}
