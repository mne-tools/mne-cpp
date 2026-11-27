//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     viewstate.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of per-viewport SubView / ViewVisibilityProfile state and QSettings round-trip.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "viewstate.h"

#include "brainsurface.h"
#include "../host/settings_store.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace mne::viz
{

//=============================================================================================================
// ViewVisibilityProfile
//=============================================================================================================

bool ViewVisibilityProfile::isObjectVisible(std::string_view object) const
{
    if (object == "lh")
        return lh;
    if (object == "rh")
        return rh;
    if (object == "bem_head")
        return bemHead;
    if (object == "bem_outer_skull")
        return bemOuterSkull;
    if (object == "bem_inner_skull")
        return bemInnerSkull;
    if (object == "sens_meg")
        return sensMeg;
    if (object == "sens_meg_grad")
        return sensMegGrad;
    if (object == "sens_meg_mag")
        return sensMegMag;
    if (object == "sens_meg_helmet")
        return sensMegHelmet;
    if (object == "sens_eeg")
        return sensEeg;
    if (object == "dig")
        return dig;
    if (object == "dig_cardinal")
        return digCardinal;
    if (object == "dig_hpi")
        return digHpi;
    if (object == "dig_eeg")
        return digEeg;
    if (object == "dig_extra")
        return digExtra;
    if (object == "field_meg")
        return megFieldMap;
    if (object == "field_eeg")
        return eegFieldMap;
    if (object == "contour_meg")
        return megFieldContours;
    if (object == "contour_eeg")
        return eegFieldContours;
    if (object == "dipoles")
        return dipoles;
    if (object == "source_space")
        return sourceSpace;
    if (object == "network")
        return network;
    if (object == "mri_slices")
        return mriSlices;
    return true; // unknown objects default visible
}

//=============================================================================================================

void ViewVisibilityProfile::setObjectVisible(std::string_view object, bool visible)
{
    if (object == "lh")
        lh = visible;
    else if (object == "rh")
        rh = visible;
    else if (object == "bem_head")
        bemHead = visible;
    else if (object == "bem_outer_skull")
        bemOuterSkull = visible;
    else if (object == "bem_inner_skull")
        bemInnerSkull = visible;
    else if (object == "sens_meg")
        sensMeg = visible;
    else if (object == "sens_meg_grad")
        sensMegGrad = visible;
    else if (object == "sens_meg_mag")
        sensMegMag = visible;
    else if (object == "sens_meg_helmet")
        sensMegHelmet = visible;
    else if (object == "sens_eeg")
        sensEeg = visible;
    else if (object == "dig")
        dig = visible;
    else if (object == "dig_cardinal")
        digCardinal = visible;
    else if (object == "dig_hpi")
        digHpi = visible;
    else if (object == "dig_eeg")
        digEeg = visible;
    else if (object == "dig_extra")
        digExtra = visible;
    else if (object == "field_meg")
        megFieldMap = visible;
    else if (object == "field_eeg")
        eegFieldMap = visible;
    else if (object == "contour_meg")
        megFieldContours = visible;
    else if (object == "contour_eeg")
        eegFieldContours = visible;
    else if (object == "dipoles")
        dipoles = visible;
    else if (object == "source_space")
        sourceSpace = visible;
    else if (object == "network")
        network = visible;
    else if (object == "mri_slices")
        mriSlices = visible;
}

//=============================================================================================================

void ViewVisibilityProfile::load(const SettingsStore& settings, const std::string& prefix)
{
    lh = settings.read_bool(prefix + "lh", lh);
    rh = settings.read_bool(prefix + "rh", rh);
    bemHead = settings.read_bool(prefix + "bemHead", bemHead);
    bemOuterSkull = settings.read_bool(prefix + "bemOuterSkull", bemOuterSkull);
    bemInnerSkull = settings.read_bool(prefix + "bemInnerSkull", bemInnerSkull);
    sensMeg = settings.read_bool(prefix + "sensMeg", sensMeg);
    sensMegGrad = settings.read_bool(prefix + "sensMegGrad", sensMegGrad);
    sensMegMag = settings.read_bool(prefix + "sensMegMag", sensMegMag);
    sensMegHelmet = settings.read_bool(prefix + "sensMegHelmet", sensMegHelmet);
    sensEeg = settings.read_bool(prefix + "sensEeg", sensEeg);
    dig = settings.read_bool(prefix + "dig", dig);
    digCardinal = settings.read_bool(prefix + "digCardinal", digCardinal);
    digHpi = settings.read_bool(prefix + "digHpi", digHpi);
    digEeg = settings.read_bool(prefix + "digEeg", digEeg);
    digExtra = settings.read_bool(prefix + "digExtra", digExtra);
    megFieldMap = settings.read_bool(prefix + "megFieldMap", megFieldMap);
    eegFieldMap = settings.read_bool(prefix + "eegFieldMap", eegFieldMap);
    megFieldContours = settings.read_bool(prefix + "megFieldContours", megFieldContours);
    eegFieldContours = settings.read_bool(prefix + "eegFieldContours", eegFieldContours);
    dipoles = settings.read_bool(prefix + "dipoles", dipoles);
    sourceSpace = settings.read_bool(prefix + "sourceSpace", sourceSpace);
    network = settings.read_bool(prefix + "network", network);
    megFieldMapOnHead = settings.read_bool(prefix + "megFieldMapOnHead", megFieldMapOnHead);
    mriSlices = settings.read_bool(prefix + "mriSlices", mriSlices);
}

//=============================================================================================================

void ViewVisibilityProfile::save(SettingsStore& settings, const std::string& prefix) const
{
    settings.set_value(prefix + "lh", lh);
    settings.set_value(prefix + "rh", rh);
    settings.set_value(prefix + "bemHead", bemHead);
    settings.set_value(prefix + "bemOuterSkull", bemOuterSkull);
    settings.set_value(prefix + "bemInnerSkull", bemInnerSkull);
    settings.set_value(prefix + "sensMeg", sensMeg);
    settings.set_value(prefix + "sensMegGrad", sensMegGrad);
    settings.set_value(prefix + "sensMegMag", sensMegMag);
    settings.set_value(prefix + "sensMegHelmet", sensMegHelmet);
    settings.set_value(prefix + "sensEeg", sensEeg);
    settings.set_value(prefix + "dig", dig);
    settings.set_value(prefix + "digCardinal", digCardinal);
    settings.set_value(prefix + "digHpi", digHpi);
    settings.set_value(prefix + "digEeg", digEeg);
    settings.set_value(prefix + "digExtra", digExtra);
    settings.set_value(prefix + "megFieldMap", megFieldMap);
    settings.set_value(prefix + "eegFieldMap", eegFieldMap);
    settings.set_value(prefix + "megFieldContours", megFieldContours);
    settings.set_value(prefix + "eegFieldContours", eegFieldContours);
    settings.set_value(prefix + "dipoles", dipoles);
    settings.set_value(prefix + "sourceSpace", sourceSpace);
    settings.set_value(prefix + "network", network);
    settings.set_value(prefix + "megFieldMapOnHead", megFieldMapOnHead);
    settings.set_value(prefix + "mriSlices", mriSlices);
}

//=============================================================================================================
// SubView
//=============================================================================================================

bool SubView::isBrainSurfaceKey(std::string_view key)
{
    if (key.empty())
        return false;
    if (key.starts_with("bem_"))
        return false;
    if (key.starts_with("sens_"))
        return false;
    if (key.starts_with("srcsp_"))
        return false;
    if (key.starts_with("dig_"))
        return false;
    return true;
}

//=============================================================================================================

bool SubView::matchesSurfaceType(std::string_view key) const
{
    return isBrainSurfaceKey(key) && key.ends_with(surfaceType);
}

//=============================================================================================================

bool SubView::shouldRenderSurface(std::string_view key) const
{
    if (key.starts_with("lh_"))
        return visibility.lh;
    if (key.starts_with("rh_"))
        return visibility.rh;

    if (key == "bem_head")
        return visibility.bemHead;
    if (key == "bem_outer_skull")
        return visibility.bemOuterSkull;
    if (key == "bem_inner_skull")
        return visibility.bemInnerSkull;

    if (key.starts_with("sens_contour_meg"))
        return visibility.megFieldMap && visibility.megFieldContours;
    if (key.starts_with("sens_contour_eeg"))
        return visibility.eegFieldMap && visibility.eegFieldContours;
    if (key.starts_with("sens_surface_meg"))
        return visibility.sensMegHelmet;
    if (key.starts_with("sens_meg_grad_"))
        return visibility.sensMeg && visibility.sensMegGrad;
    if (key.starts_with("sens_meg_mag_"))
        return visibility.sensMeg && visibility.sensMegMag;
    if (key.starts_with("sens_meg_"))
        return visibility.sensMeg;
    if (key.starts_with("sens_eeg_"))
        return visibility.sensEeg;

    if (key.starts_with("dig_live_"))
        return true; // always show live tracker
    if (key.starts_with("dig_static_"))
        return true; // always show static markers
    if (key.starts_with("dig_ray_"))
        return true; // always show optical ray
    if (key.starts_with("dig_probe_"))
        return true; // always show catheter probe
    if (key.starts_with("dig_cardinal"))
        return visibility.dig && visibility.digCardinal;
    if (key.starts_with("dig_hpi"))
        return visibility.dig && visibility.digHpi;
    if (key.starts_with("dig_eeg"))
        return visibility.dig && visibility.digEeg;
    if (key.starts_with("dig_extra"))
        return visibility.dig && visibility.digExtra;
    if (key.starts_with("dig_"))
        return visibility.dig;

    if (key.starts_with("srcsp_"))
        return visibility.sourceSpace;

    return true;
}

//=============================================================================================================

void SubView::applyOverlayToSurfaces(
    std::map<std::string, std::shared_ptr<BrainSurface>>& surfaces) const
{
    for (auto& [key, surface] : surfaces) {
        if (matchesSurfaceType(key)) {
            surface->setVisualizationMode(overlayMode);
        }
    }
}

//=============================================================================================================
// Camera presets
//=============================================================================================================

Eigen::Quaternionf perspectivePresetRotation()
{
    return viz::perspective_preset_rotation();
}

//=============================================================================================================

std::string multiViewPresetName(int preset)
{
    return std::string(viz::view_preset_name(preset));
}

//=============================================================================================================

Eigen::Quaternionf multiViewPresetOffset(int preset)
{
    return viz::view_preset_rotation(preset);
}

//=============================================================================================================

bool multiViewPresetIsPerspective(int preset)
{
    return viz::view_preset_is_perspective(preset);
}

//=============================================================================================================
// SubView factory
//=============================================================================================================

SubView SubView::defaultForIndex(int index)
{
    static constexpr ShaderMode kShaderCycle[] = {
        Anatomical,
        Standard,
        Holographic,
    };
    static constexpr int kNumShaders = 3;
    static constexpr int kNumPresets = 7; // Top..Right

    SubView sv;
    sv.preset = index % kNumPresets;
    sv.brainShader = kShaderCycle[index % kNumShaders];
    sv.enabled = true;
    return sv;
}

//=============================================================================================================
// Enum ↔ string conversion
//=============================================================================================================

int normalizedVisualizationTarget(int target, int maxIndex)
{
    return std::clamp(target, -1, maxIndex);
}

//=============================================================================================================

ShaderMode shaderModeFromName(std::string_view name)
{
    if (name == "Holographic")
        return Holographic;
    if (name == "Anatomical")
        return Anatomical;
    if (name == "XRay")
        return XRay;
    return Standard;
}

//=============================================================================================================

std::string shaderModeName(ShaderMode mode)
{
    if (mode == Holographic)
        return "Holographic";
    if (mode == Anatomical)
        return "Anatomical";
    return "Standard";
}

//=============================================================================================================

VisualizationMode visualizationModeFromName(std::string_view name)
{
    if (name == "Annotation")
        return ModeAnnotation;
    if (name == "Scientific")
        return ModeScientific;
    if (name == "Source Estimate")
        return ModeSourceEstimate;
    return ModeSurface;
}

//=============================================================================================================

std::string visualizationModeName(VisualizationMode mode)
{
    if (mode == ModeAnnotation)
        return "Annotation";
    if (mode == ModeScientific)
        return "Scientific";
    if (mode == ModeSourceEstimate)
        return "Source Estimate";
    return "Surface";
}

//=============================================================================================================
// Colormap
//=============================================================================================================

Color mneAnalyzeColor(double v)
{
    double x = 2.0 * v - 1.0;
    x = std::clamp(x, -1.0, 1.0);

    static constexpr int N = 7;
    static const double pos[N] = {-1.0, -0.90, -0.30, 0.0, 0.30, 0.90, 1.0};
    static const double rr[N] = {0.0, 0.0, 0.5, 0.5, 0.5, 1.0, 1.0};
    static const double gg[N] = {1.0, 0.0, 0.5, 0.5, 0.5, 0.0, 1.0};
    static const double bb[N] = {1.0, 1.0, 0.5, 0.5, 0.5, 0.0, 0.0};

    int seg = 0;
    for (int i = 0; i < N - 1; ++i) {
        if (x >= pos[i] && x <= pos[i + 1]) {
            seg = i;
            break;
        }
    }
    if (x > pos[N - 1])
        seg = N - 2;

    double t = (pos[seg + 1] != pos[seg])
        ? (x - pos[seg]) / (pos[seg + 1] - pos[seg])
        : 0.0;
    t = std::clamp(t, 0.0, 1.0);

    int r = static_cast<int>(std::round((rr[seg] + t * (rr[seg + 1] - rr[seg])) * 255.0));
    int g = static_cast<int>(std::round((gg[seg] + t * (gg[seg + 1] - gg[seg])) * 255.0));
    int b = static_cast<int>(std::round((bb[seg] + t * (bb[seg + 1] - bb[seg])) * 255.0));

    return {static_cast<std::uint8_t>(std::clamp(r, 0, 255)), static_cast<std::uint8_t>(std::clamp(g, 0, 255)), static_cast<std::uint8_t>(std::clamp(b, 0, 255)), 255};
}

//=============================================================================================================
// SubView serialisation
//=============================================================================================================

void SubView::load(const SettingsStore& settings, const std::string& prefix,
                   const Eigen::Quaternionf& fallbackRotation)
{
    surfaceType = settings.read_string(prefix + "surfaceType", surfaceType);
    brainShader = shaderModeFromName(settings.read_string(prefix + "shader", shaderModeName(brainShader)));
    bemShader = shaderModeFromName(settings.read_string(prefix + "bemShader", shaderModeName(bemShader)));
    overlayMode = visualizationModeFromName(settings.read_string(prefix + "overlay", visualizationModeName(overlayMode)));

    visibility.load(settings, prefix + "vis_");

    zoom = static_cast<float>(settings.read_double(prefix + "zoom", zoom));
    pan = Eigen::Vector2f(static_cast<float>(settings.read_double(prefix + "panX", pan.x())), static_cast<float>(settings.read_double(prefix + "panY", pan.y())));
    preset = viz::clamp_view_preset(settings.read_int(prefix + "preset", preset));

    const bool hasQuat = settings.contains(prefix + "perspRotW") && settings.contains(prefix + "perspRotX") && settings.contains(prefix + "perspRotY") && settings.contains(prefix + "perspRotZ");
    if (hasQuat) {
        const auto component = [&](const char* name, double fallback) {
            return static_cast<float>(settings.read_double(prefix + name, fallback));
        };
        rotation = Eigen::Quaternionf(component("perspRotW", 1.0), component("perspRotX", 0.0), component("perspRotY", 0.0), component("perspRotZ", 0.0));
        if (rotation.squaredNorm() <= std::numeric_limits<float>::epsilon()) {
            rotation = Eigen::Quaternionf::Identity();
        } else {
            rotation.normalize();
        }
    } else {
        rotation = fallbackRotation;
    }
}

//=============================================================================================================

void SubView::save(SettingsStore& settings, const std::string& prefix) const
{
    settings.set_value(prefix + "surfaceType", surfaceType);
    settings.set_value(prefix + "shader", shaderModeName(brainShader));
    settings.set_value(prefix + "bemShader", shaderModeName(bemShader));
    settings.set_value(prefix + "overlay", visualizationModeName(overlayMode));

    visibility.save(settings, prefix + "vis_");

    settings.set_value(prefix + "zoom", static_cast<double>(zoom));
    settings.set_value(prefix + "panX", static_cast<double>(pan.x()));
    settings.set_value(prefix + "panY", static_cast<double>(pan.y()));
    settings.set_value(prefix + "preset", preset);

    settings.set_value(prefix + "perspRotW", static_cast<double>(rotation.w()));
    settings.set_value(prefix + "perspRotX", static_cast<double>(rotation.x()));
    settings.set_value(prefix + "perspRotY", static_cast<double>(rotation.y()));
    settings.set_value(prefix + "perspRotZ", static_cast<double>(rotation.z()));
}

} // namespace mne::viz
