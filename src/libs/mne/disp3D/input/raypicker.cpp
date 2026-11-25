//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     raypicker.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Mouse-ray unprojection plus Moller-Trumbore triangle and sphere / cylinder intersection over all selectable scene primitives.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "raypicker.h"

#include "renderable/brainsurface.h"
#include "renderable/dipoleobject.h"
#include "model/items/abstracttreeitem.h"
#include "model/items/digitizertreeitem.h"
#include "core/qteigen_p.h"

#include <mne/scene/ray_picking.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cctype>
#include <format>
#include <iterator>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace mne::disp
{

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

bool RayPicker::unproject(const QPoint& screenPos,
                          const QRect& paneRect,
                          const QMatrix4x4& pvm,
                          QVector3D& rayOrigin,
                          QVector3D& rayDir)
{
    const std::optional<scene::Ray> ray = scene::ray_from_pixel(
        {static_cast<double>(screenPos.x()), static_cast<double>(screenPos.y())},
        {static_cast<double>(paneRect.x()), static_cast<double>(paneRect.y()), static_cast<double>(paneRect.width()),
         static_cast<double>(paneRect.height())},
        toEigen(pvm));
    if (!ray) {
        return false;
    }
    rayOrigin = toQVector3D(ray->origin);
    rayDir = toQVector3D(ray->direction);
    return true;
}

//=============================================================================================================

RayHit RayPicker::pick(const QVector3D& rayOrigin,
                       const QVector3D& rayDir,
                       const SubView& subView,
                       const std::map<std::string, std::shared_ptr<BrainSurface>>& surfaces,
                       const std::map<const QStandardItem*, std::shared_ptr<BrainSurface>>& itemSurfaceMap,
                       const std::map<const QStandardItem*, std::shared_ptr<DipoleObject>>& itemDipoleMap)
{
    RayHit result;
    float closestDist = std::numeric_limits<float>::max();

    // ── Test surfaces ──────────────────────────────────────────────────
    for (const auto& [key, surf] : surfaces) {
        if (!surf->isVisible())
            continue;
        if (!subView.shouldRenderSurface(key))
            continue;
        if (key.starts_with("srcsp_"))
            continue; // skip source space for picking

        const bool isSensor = key.starts_with("sens_");
        const bool isBem = key.starts_with("bem_");
        const bool isDig = key.starts_with("dig_");

        // Brain surfaces: only pick if matching active surface type
        if (!isSensor && !isBem && !isDig) {
            if (!subView.matchesSurfaceType(key))
                continue;
        }

        float dist = 0.0f;
        int vertexIdx = -1;
        if (surf->intersects(rayOrigin, rayDir, dist, vertexIdx)) {
            if (dist < closestDist) {
                closestDist = dist;
                result.hit = true;
                result.distance = dist;
                result.hitPoint = rayOrigin + dist * rayDir;
                result.vertexIndex = vertexIdx;
                result.surfaceKey = key;
                result.isDipole = false;
                result.dipoleIndex = -1;

                // Reverse lookup: find tree item for this surface
                result.item = nullptr;
                for (const auto& [item, itemSurface] : itemSurfaceMap) {
                    if (itemSurface == surf) {
                        result.item = const_cast<QStandardItem*>(item);
                        break;
                    }
                }

                // FsAnnotation info
                const auto itemSurface = result.item ? itemSurfaceMap.find(result.item) : itemSurfaceMap.end();
                if (itemSurface != itemSurfaceMap.end()) {
                    result.regionName = itemSurface->second->getAnnotationLabel(vertexIdx);
                    result.regionId = itemSurface->second->getAnnotationLabelId(vertexIdx);
                } else {
                    result.regionName.clear();
                    result.regionId = -1;
                }
            }
        }
    }

    // ── Test dipoles ───────────────────────────────────────────────────
    for (const auto& [item, dipole] : itemDipoleMap) {
        if (!subView.visibility.dipoles)
            continue;
        if (!dipole->isVisible())
            continue;

        float dist = 0.0f;
        int dipIdx = dipole->intersect(rayOrigin, rayDir, dist);
        if (dipIdx != -1 && dist < closestDist) {
            closestDist = dist;
            result.hit = true;
            result.distance = dist;
            result.hitPoint = rayOrigin + dist * rayDir;
            result.item = const_cast<QStandardItem*>(item);
            result.surfaceKey.clear();
            result.vertexIndex = dipIdx;
            result.isDipole = true;
            result.dipoleIndex = dipIdx;
            result.regionName.clear();
            result.regionId = -1;
        }
    }

    return result;
}

//=============================================================================================================

std::string RayPicker::buildLabel(const RayHit& result,
                                  [[maybe_unused]] const std::map<const QStandardItem*, std::shared_ptr<BrainSurface>>& itemSurfaceMap,
                                  [[maybe_unused]] const std::map<std::string, std::shared_ptr<BrainSurface>>& surfaces)
{
    if (!result.hit)
        return {};

    const std::string& key = result.surfaceKey;
    const std::string itemText = result.item ? result.item->text().toStdString() : std::string();
    const auto capitalized = [](std::string text) {
        if (!text.empty())
            text[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(text[0])));
        return text;
    };

    // ── Brain surface with annotation ──────────────────────────────────
    if (!result.regionName.empty()) {
        return result.displayLabel();
    }

    // ── Dipole ─────────────────────────────────────────────────────────
    if (result.isDipole) {
        return std::format("{} (Dipole {})", result.item ? itemText : std::string("Dipole"), result.dipoleIndex);
    }

    // ── Sensor/BEM/Digitizer/Helmet ────────────────────────────────────
    if (key.starts_with("sens_surface_meg")) {
        return "MEG Helmet";
    }
    if (key.starts_with("sens_meg_")) {
        return "MEG: " + (result.item ? itemText : key);
    }
    if (key.starts_with("sens_eeg_")) {
        return "EEG: " + (result.item ? itemText : key);
    }
    if (key.starts_with("dig_")) {
        // Resolve individual point from batched mesh
        std::string pointName;
        if (result.item && result.vertexIndex >= 0) {
            AbstractTreeItem* abs = dynamic_cast<AbstractTreeItem*>(result.item);
            if (abs && abs->type() == AbstractTreeItem::itemTypeId(AbstractTreeItem::DigitizerItem)) {
                auto* digItem = static_cast<DigitizerTreeItem*>(abs);
                constexpr int vertsPerSphere = 42;
                int ptIdx = result.vertexIndex / vertsPerSphere;
                const std::vector<std::string>& names = digItem->pointNames();
                if (ptIdx >= 0 && ptIdx < std::ssize(names))
                    pointName = names[static_cast<std::size_t>(ptIdx)];
            }
        }
        const std::string category = capitalized(key.substr(4));
        return pointName.empty()
            ? std::format("Digitizer ({})", category)
            : std::format("Digitizer: {} ({})", pointName, category);
    }
    if (key.starts_with("bem_")) {
        std::string compartment = capitalized(key.substr(4));
        std::ranges::replace(compartment, '_', ' ');
        return "BEM: " + compartment;
    }

    // ── Hemisphere fallback ────────────────────────────────────────────
    if (key.starts_with("lh_"))
        return "Left Hemisphere";
    if (key.starts_with("rh_"))
        return "Right Hemisphere";

    return {};
}

//=============================================================================================================

std::string RayHit::displayLabel() const
{
    // Delegated to the static builder in RayPicker; this method is a
    // convenience wrapper when the caller doesn't have the surface maps.
    if (!hit)
        return {};

    if (!regionName.empty()) {
        if (surfaceKey.starts_with("lh"))
            return std::format("Region: {} (lh)", regionName);
        if (surfaceKey.starts_with("rh"))
            return std::format("Region: {} (rh)", regionName);
        return "Region: " + regionName;
    }

    if (isDipole) {
        return std::format("Dipole {}", dipoleIndex);
    }

    return surfaceKey;
}

} // namespace mne::disp
