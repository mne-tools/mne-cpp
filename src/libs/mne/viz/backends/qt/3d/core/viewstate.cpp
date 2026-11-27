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

#include "../../../../brain/brainsurface.h"
#include "../view/brainrenderer.h"
#include "../../qtconversions.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSettings>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <limits>

namespace mne::viz::qt
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

void ViewVisibilityProfile::load(const QSettings& settings, const QString& prefix)
{
    lh = isTrue(settings.value(prefix + "lh"), lh);
    rh = isTrue(settings.value(prefix + "rh"), rh);
    bemHead = isTrue(settings.value(prefix + "bemHead"), bemHead);
    bemOuterSkull = isTrue(settings.value(prefix + "bemOuterSkull"), bemOuterSkull);
    bemInnerSkull = isTrue(settings.value(prefix + "bemInnerSkull"), bemInnerSkull);
    sensMeg = isTrue(settings.value(prefix + "sensMeg"), sensMeg);
    sensMegGrad = isTrue(settings.value(prefix + "sensMegGrad"), sensMegGrad);
    sensMegMag = isTrue(settings.value(prefix + "sensMegMag"), sensMegMag);
    sensMegHelmet = isTrue(settings.value(prefix + "sensMegHelmet"), sensMegHelmet);
    sensEeg = isTrue(settings.value(prefix + "sensEeg"), sensEeg);
    dig = isTrue(settings.value(prefix + "dig"), dig);
    digCardinal = isTrue(settings.value(prefix + "digCardinal"), digCardinal);
    digHpi = isTrue(settings.value(prefix + "digHpi"), digHpi);
    digEeg = isTrue(settings.value(prefix + "digEeg"), digEeg);
    digExtra = isTrue(settings.value(prefix + "digExtra"), digExtra);
    megFieldMap = isTrue(settings.value(prefix + "megFieldMap"), megFieldMap);
    eegFieldMap = isTrue(settings.value(prefix + "eegFieldMap"), eegFieldMap);
    megFieldContours = isTrue(settings.value(prefix + "megFieldContours"), megFieldContours);
    eegFieldContours = isTrue(settings.value(prefix + "eegFieldContours"), eegFieldContours);
    dipoles = isTrue(settings.value(prefix + "dipoles"), dipoles);
    sourceSpace = isTrue(settings.value(prefix + "sourceSpace"), sourceSpace);
    network = isTrue(settings.value(prefix + "network"), network);
    megFieldMapOnHead = isTrue(settings.value(prefix + "megFieldMapOnHead"), megFieldMapOnHead);
    mriSlices = isTrue(settings.value(prefix + "mriSlices"), mriSlices);
}

//=============================================================================================================

void ViewVisibilityProfile::save(QSettings& settings, const QString& prefix) const
{
    settings.setValue(prefix + "lh", lh);
    settings.setValue(prefix + "rh", rh);
    settings.setValue(prefix + "bemHead", bemHead);
    settings.setValue(prefix + "bemOuterSkull", bemOuterSkull);
    settings.setValue(prefix + "bemInnerSkull", bemInnerSkull);
    settings.setValue(prefix + "sensMeg", sensMeg);
    settings.setValue(prefix + "sensMegGrad", sensMegGrad);
    settings.setValue(prefix + "sensMegMag", sensMegMag);
    settings.setValue(prefix + "sensMegHelmet", sensMegHelmet);
    settings.setValue(prefix + "sensEeg", sensEeg);
    settings.setValue(prefix + "dig", dig);
    settings.setValue(prefix + "digCardinal", digCardinal);
    settings.setValue(prefix + "digHpi", digHpi);
    settings.setValue(prefix + "digEeg", digEeg);
    settings.setValue(prefix + "digExtra", digExtra);
    settings.setValue(prefix + "megFieldMap", megFieldMap);
    settings.setValue(prefix + "eegFieldMap", eegFieldMap);
    settings.setValue(prefix + "megFieldContours", megFieldContours);
    settings.setValue(prefix + "eegFieldContours", eegFieldContours);
    settings.setValue(prefix + "dipoles", dipoles);
    settings.setValue(prefix + "sourceSpace", sourceSpace);
    settings.setValue(prefix + "network", network);
    settings.setValue(prefix + "megFieldMapOnHead", megFieldMapOnHead);
    settings.setValue(prefix + "mriSlices", mriSlices);
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

QQuaternion perspectivePresetRotation()
{
    return toQQuaternion(viz::perspective_preset_rotation());
}

//=============================================================================================================

std::string multiViewPresetName(int preset)
{
    return std::string(viz::view_preset_name(preset));
}

//=============================================================================================================

QQuaternion multiViewPresetOffset(int preset)
{
    return toQQuaternion(viz::view_preset_rotation(preset));
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

QRgb mneAnalyzeColor(double v)
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

    return qRgb(std::clamp(r, 0, 255), std::clamp(g, 0, 255), std::clamp(b, 0, 255));
}

//=============================================================================================================
// SubView serialisation
//=============================================================================================================

void SubView::load(const QSettings& settings, const QString& prefix,
                   const Eigen::Quaternionf& fallbackRotation)
{
    surfaceType = settings.value(prefix + "surfaceType", QString::fromStdString(surfaceType)).toString().toStdString();
    brainShader = shaderModeFromName(settings.value(prefix + "shader", QString::fromStdString(shaderModeName(brainShader))).toString().toStdString());
    bemShader = shaderModeFromName(settings.value(prefix + "bemShader", QString::fromStdString(shaderModeName(bemShader))).toString().toStdString());
    overlayMode = visualizationModeFromName(settings.value(prefix + "overlay", QString::fromStdString(visualizationModeName(overlayMode))).toString().toStdString());

    visibility.load(settings, prefix + "vis_");

    zoom = settings.value(prefix + "zoom", zoom).toFloat();
    pan = Eigen::Vector2f(settings.value(prefix + "panX", pan.x()).toFloat(), settings.value(prefix + "panY", pan.y()).toFloat());
    preset = viz::clamp_view_preset(settings.value(prefix + "preset", preset).toInt());

    const bool hasQuat = settings.contains(prefix + "perspRotW") && settings.contains(prefix + "perspRotX") && settings.contains(prefix + "perspRotY") && settings.contains(prefix + "perspRotZ");
    if (hasQuat) {
        const float w = settings.value(prefix + "perspRotW", 1.0f).toFloat();
        const float x = settings.value(prefix + "perspRotX", 0.0f).toFloat();
        const float y = settings.value(prefix + "perspRotY", 0.0f).toFloat();
        const float z = settings.value(prefix + "perspRotZ", 0.0f).toFloat();
        rotation = Eigen::Quaternionf(w, x, y, z);
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

void SubView::save(QSettings& settings, const QString& prefix) const
{
    settings.setValue(prefix + "surfaceType", QString::fromStdString(surfaceType));
    settings.setValue(prefix + "shader", QString::fromStdString(shaderModeName(brainShader)));
    settings.setValue(prefix + "bemShader", QString::fromStdString(shaderModeName(bemShader)));
    settings.setValue(prefix + "overlay", QString::fromStdString(visualizationModeName(overlayMode)));

    visibility.save(settings, prefix + "vis_");

    settings.setValue(prefix + "zoom", zoom);
    settings.setValue(prefix + "panX", pan.x());
    settings.setValue(prefix + "panY", pan.y());
    settings.setValue(prefix + "preset", preset);

    settings.setValue(prefix + "perspRotW", rotation.w());
    settings.setValue(prefix + "perspRotX", rotation.x());
    settings.setValue(prefix + "perspRotY", rotation.y());
    settings.setValue(prefix + "perspRotZ", rotation.z());
}

} // namespace mne::viz::qt
