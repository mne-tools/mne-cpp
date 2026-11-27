//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     surfacekeys.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    String key constants for surfaces in the scene map of the brain view.
 *
 * BrainView and the scene controllers store all renderable surfaces
 * in a single @c std::map<std::string, std::shared_ptr<BrainSurface>>. The
 * keys in this namespace are the canonical identifiers used by every
 * controller to look up, replace or remove a surface without race
 * conditions caused by typo'd magic strings.
 *
 * Keys cover the two cortical hemispheres (@c lh / @c rh on each of
 * pial, white, inflated, smoothwm), the three BEM compartments
 * (brain, inner-skull, outer-skull, scalp), MEG helmet / iso-contour
 * surfaces, and a few diagnostic primitives (origin marker,
 * orientation gizmo).
 */

#ifndef VIZ_SURFACEKEYS_H
#define VIZ_SURFACEKEYS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>
#include <string_view>
#include <unordered_map>

//=============================================================================================================
/**
 * @namespace mne::viz::surface_keys
 * @brief     Centralised string constants for surface map keys.
 *
 * Every surface stored in BrainView's `m_surfaces` map is identified by a
 * string key formed from one of these prefixes plus an object-specific
 * suffix.  Using these constants instead of raw string literals ensures
 * consistency and enables compile-time typo detection.
 */
namespace mne::viz::surface_keys
{
// ── Hemisphere prefixes ────────────────────────────────────────────
inline constexpr std::string_view kLhPrefix = "lh_";
inline constexpr std::string_view kRhPrefix = "rh_";

// ── BEM ────────────────────────────────────────────────────────────
inline constexpr std::string_view kBemPrefix = "bem_";
inline constexpr std::string_view kBemHead = "bem_head";

// ── Sensors ────────────────────────────────────────────────────────
inline constexpr std::string_view kSensPrefix = "sens_";
inline constexpr std::string_view kSensMegGrad = "sens_meg_grad_";
inline constexpr std::string_view kSensMegMag = "sens_meg_mag_";
inline constexpr std::string_view kSensMeg = "sens_meg_";
inline constexpr std::string_view kSensEeg = "sens_eeg_";
inline constexpr std::string_view kSensDig = "sens_dig_";
inline constexpr std::string_view kHelmet = "sens_surface_meg";

// ── Digitiser ──────────────────────────────────────────────────────
inline constexpr std::string_view kDigPrefix = "dig_";
inline constexpr std::string_view kDigCardinal = "dig_cardinal";
inline constexpr std::string_view kDigHpi = "dig_hpi";
inline constexpr std::string_view kDigEeg = "dig_eeg";
inline constexpr std::string_view kDigExtra = "dig_extra";

// ── Source space ───────────────────────────────────────────────────
inline constexpr std::string_view kSrcSpPrefix = "srcsp_";

// ── Contour overlays ───────────────────────────────────────────────
inline constexpr std::string_view kContourMeg = "sens_contour_meg";
inline constexpr std::string_view kContourEeg = "sens_contour_eeg";

// ── Inflated surface keys ──────────────────────────────────────────
inline constexpr std::string_view kLhInflated = "lh_inflated";
inline constexpr std::string_view kRhInflated = "rh_inflated";

//=============================================================================================================
/**
 * Map from user-facing sensor type name (e.g. "MEG/Grad") to the
 * visibility-profile object key (e.g. "sens_meg_grad").
 *
 * The map is built once on first call and cached.
 *
 * @param[in] uiType   UI-facing type string.
 * @return              Corresponding object key, or empty string if unknown.
 */
inline std::string sensorTypeToObjectKey(std::string_view uiType)
{
    static const std::unordered_map<std::string_view, std::string_view> map = {
        {"MEG", "sens_meg"},
        {"MEG/Grad", "sens_meg_grad"},
        {"MEG/Mag", "sens_meg_mag"},
        {"MEG Helmet", "sens_meg_helmet"},
        {"EEG", "sens_eeg"},
        {"Digitizer", "dig"},
        {"Digitizer/Cardinal", "dig_cardinal"},
        {"Digitizer/HPI", "dig_hpi"},
        {"Digitizer/EEG", "dig_eeg"},
        {"Digitizer/Extra", "dig_extra"},
    };
    const auto it = map.find(uiType);
    return it == map.end() ? std::string() : std::string(it->second);
}

//=============================================================================================================
/**
 * Map a parent-item display text (e.g. "MEG/Grad") to the surface-map key
 * prefix used when registering individual sensor surfaces.
 *
 * @param[in] parentText   Text of the parent tree item.
 * @return                 Key prefix string.
 */
inline std::string sensorParentToKeyPrefix(std::string_view parentText)
{
    if (parentText.contains("MEG/Grad"))
        return "sens_meg_grad_";
    if (parentText.contains("MEG/Mag"))
        return "sens_meg_mag_";
    if (parentText.contains("MEG"))
        return "sens_meg_";
    if (parentText.contains("EEG"))
        return "sens_eeg_";
    if (parentText.contains("Digitizer"))
        return "sens_dig_";
    return "sens_";
}

} // namespace mne::viz::surface_keys

#endif // VIZ_SURFACEKEYS_H
