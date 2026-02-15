//=============================================================================================================
/**
 * @file     viewstate.cpp
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
 * @brief    ViewState implementations — per-view data structures and helpers.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "viewstate.h"

#include <QSettings>
#include <cmath>
#include <limits>

//=============================================================================================================
// ViewVisibilityProfile
//=============================================================================================================

bool ViewVisibilityProfile::isObjectVisible(const QString &object) const
{
    if (object == "lh")                return lh;
    if (object == "rh")                return rh;
    if (object == "bem_head")          return bemHead;
    if (object == "bem_outer_skull")   return bemOuterSkull;
    if (object == "bem_inner_skull")   return bemInnerSkull;
    if (object == "sens_meg")          return sensMeg;
    if (object == "sens_meg_grad")     return sensMegGrad;
    if (object == "sens_meg_mag")      return sensMegMag;
    if (object == "sens_meg_helmet")   return sensMegHelmet;
    if (object == "sens_eeg")          return sensEeg;
    if (object == "dig")               return dig;
    if (object == "dig_cardinal")      return digCardinal;
    if (object == "dig_hpi")           return digHpi;
    if (object == "dig_eeg")           return digEeg;
    if (object == "dig_extra")         return digExtra;
    if (object == "field_meg")         return megFieldMap;
    if (object == "field_eeg")         return eegFieldMap;
    if (object == "contour_meg")       return megFieldContours;
    if (object == "contour_eeg")       return eegFieldContours;
    if (object == "dipoles")           return dipoles;
    if (object == "source_space")      return sourceSpace;
    if (object == "network")           return network;
    return true; // unknown objects default visible
}

//=============================================================================================================

void ViewVisibilityProfile::setObjectVisible(const QString &object, bool visible)
{
    if      (object == "lh")                lh = visible;
    else if (object == "rh")                rh = visible;
    else if (object == "bem_head")          bemHead = visible;
    else if (object == "bem_outer_skull")   bemOuterSkull = visible;
    else if (object == "bem_inner_skull")   bemInnerSkull = visible;
    else if (object == "sens_meg")          sensMeg = visible;
    else if (object == "sens_meg_grad")     sensMegGrad = visible;
    else if (object == "sens_meg_mag")      sensMegMag = visible;
    else if (object == "sens_meg_helmet")   sensMegHelmet = visible;
    else if (object == "sens_eeg")          sensEeg = visible;
    else if (object == "dig")              dig = visible;
    else if (object == "dig_cardinal")     digCardinal = visible;
    else if (object == "dig_hpi")          digHpi = visible;
    else if (object == "dig_eeg")          digEeg = visible;
    else if (object == "dig_extra")        digExtra = visible;
    else if (object == "field_meg")        megFieldMap = visible;
    else if (object == "field_eeg")        eegFieldMap = visible;
    else if (object == "contour_meg")      megFieldContours = visible;
    else if (object == "contour_eeg")      eegFieldContours = visible;
    else if (object == "dipoles")          dipoles = visible;
    else if (object == "source_space")     sourceSpace = visible;
    else if (object == "network")          network = visible;
}

//=============================================================================================================

void ViewVisibilityProfile::load(const QSettings &settings, const QString &prefix)
{
    lh              = isTrue(settings.value(prefix + "lh"),              lh);
    rh              = isTrue(settings.value(prefix + "rh"),              rh);
    bemHead         = isTrue(settings.value(prefix + "bemHead"),         bemHead);
    bemOuterSkull   = isTrue(settings.value(prefix + "bemOuterSkull"),   bemOuterSkull);
    bemInnerSkull   = isTrue(settings.value(prefix + "bemInnerSkull"),   bemInnerSkull);
    sensMeg         = isTrue(settings.value(prefix + "sensMeg"),         sensMeg);
    sensMegGrad     = isTrue(settings.value(prefix + "sensMegGrad"),     sensMegGrad);
    sensMegMag      = isTrue(settings.value(prefix + "sensMegMag"),      sensMegMag);
    sensMegHelmet   = isTrue(settings.value(prefix + "sensMegHelmet"),   sensMegHelmet);
    sensEeg         = isTrue(settings.value(prefix + "sensEeg"),         sensEeg);
    dig             = isTrue(settings.value(prefix + "dig"),             dig);
    digCardinal     = isTrue(settings.value(prefix + "digCardinal"),     digCardinal);
    digHpi          = isTrue(settings.value(prefix + "digHpi"),          digHpi);
    digEeg          = isTrue(settings.value(prefix + "digEeg"),          digEeg);
    digExtra        = isTrue(settings.value(prefix + "digExtra"),        digExtra);
    megFieldMap     = isTrue(settings.value(prefix + "megFieldMap"),     megFieldMap);
    eegFieldMap     = isTrue(settings.value(prefix + "eegFieldMap"),     eegFieldMap);
    megFieldContours = isTrue(settings.value(prefix + "megFieldContours"), megFieldContours);
    eegFieldContours = isTrue(settings.value(prefix + "eegFieldContours"), eegFieldContours);
    dipoles         = isTrue(settings.value(prefix + "dipoles"),         dipoles);
    sourceSpace     = isTrue(settings.value(prefix + "sourceSpace"),     sourceSpace);
    network         = isTrue(settings.value(prefix + "network"),         network);
    megFieldMapOnHead = isTrue(settings.value(prefix + "megFieldMapOnHead"), megFieldMapOnHead);
}

//=============================================================================================================

void ViewVisibilityProfile::save(QSettings &settings, const QString &prefix) const
{
    settings.setValue(prefix + "lh",              lh);
    settings.setValue(prefix + "rh",              rh);
    settings.setValue(prefix + "bemHead",         bemHead);
    settings.setValue(prefix + "bemOuterSkull",   bemOuterSkull);
    settings.setValue(prefix + "bemInnerSkull",   bemInnerSkull);
    settings.setValue(prefix + "sensMeg",         sensMeg);
    settings.setValue(prefix + "sensMegGrad",     sensMegGrad);
    settings.setValue(prefix + "sensMegMag",      sensMegMag);
    settings.setValue(prefix + "sensMegHelmet",   sensMegHelmet);
    settings.setValue(prefix + "sensEeg",         sensEeg);
    settings.setValue(prefix + "dig",             dig);
    settings.setValue(prefix + "digCardinal",     digCardinal);
    settings.setValue(prefix + "digHpi",          digHpi);
    settings.setValue(prefix + "digEeg",          digEeg);
    settings.setValue(prefix + "digExtra",        digExtra);
    settings.setValue(prefix + "megFieldMap",     megFieldMap);
    settings.setValue(prefix + "eegFieldMap",     eegFieldMap);
    settings.setValue(prefix + "megFieldContours", megFieldContours);
    settings.setValue(prefix + "eegFieldContours", eegFieldContours);
    settings.setValue(prefix + "dipoles",         dipoles);
    settings.setValue(prefix + "sourceSpace",     sourceSpace);
    settings.setValue(prefix + "network",         network);
    settings.setValue(prefix + "megFieldMapOnHead", megFieldMapOnHead);
}

//=============================================================================================================
// SubView
//=============================================================================================================

bool SubView::isBrainSurfaceKey(const QString &key)
{
    if (key.startsWith("bem_"))   return false;
    if (key.startsWith("sens_"))  return false;
    if (key.startsWith("srcsp_")) return false;
    if (key.startsWith("dig_"))   return false;
    return true;
}

//=============================================================================================================

bool SubView::matchesSurfaceType(const QString &key) const
{
    return isBrainSurfaceKey(key) && key.endsWith(surfaceType);
}

//=============================================================================================================

bool SubView::shouldRenderSurface(const QString &key) const
{
    if (key.startsWith("lh_")) return visibility.lh;
    if (key.startsWith("rh_")) return visibility.rh;

    if (key == "bem_head")        return visibility.bemHead;
    if (key == "bem_outer_skull") return visibility.bemOuterSkull;
    if (key == "bem_inner_skull") return visibility.bemInnerSkull;

    if (key.startsWith("sens_contour_meg")) return visibility.megFieldMap && visibility.megFieldContours;
    if (key.startsWith("sens_contour_eeg")) return visibility.eegFieldMap && visibility.eegFieldContours;
    if (key.startsWith("sens_surface_meg")) return visibility.sensMeg && visibility.sensMegHelmet;
    if (key.startsWith("sens_meg_grad_"))   return visibility.sensMeg && visibility.sensMegGrad;
    if (key.startsWith("sens_meg_mag_"))    return visibility.sensMeg && visibility.sensMegMag;
    if (key.startsWith("sens_meg_"))        return visibility.sensMeg;
    if (key.startsWith("sens_eeg_"))        return visibility.sensEeg;

    if (key.startsWith("dig_cardinal")) return visibility.dig && visibility.digCardinal;
    if (key.startsWith("dig_hpi"))      return visibility.dig && visibility.digHpi;
    if (key.startsWith("dig_eeg"))      return visibility.dig && visibility.digEeg;
    if (key.startsWith("dig_extra"))    return visibility.dig && visibility.digExtra;
    if (key.startsWith("dig_"))         return visibility.dig;

    if (key.startsWith("srcsp_")) return visibility.sourceSpace;

    return true;
}

//=============================================================================================================

void SubView::applyOverlayToSurfaces(
    QMap<QString, std::shared_ptr<BrainSurface>> &surfaces) const
{
    for (auto it = surfaces.begin(); it != surfaces.end(); ++it) {
        if (matchesSurfaceType(it.key())) {
            it.value()->setVisualizationMode(overlayMode);
        }
    }
}

//=============================================================================================================
// Camera presets
//=============================================================================================================

QQuaternion perspectivePresetRotation()
{
    return QQuaternion::fromEulerAngles(-45.0f, -40.0f, -130.0f);
}

//=============================================================================================================

QString multiViewPresetName(int preset)
{
    switch (preset) {
    case 0: return QStringLiteral("Top");
    case 1: return QStringLiteral("Perspective");
    case 2: return QStringLiteral("Front");
    case 3: return QStringLiteral("Left");
    case 4: return QStringLiteral("Bottom");
    case 5: return QStringLiteral("Back");
    case 6: return QStringLiteral("Right");
    default: return QStringLiteral("Top");
    }
}

//=============================================================================================================

QQuaternion multiViewPresetOffset(int preset)
{
    switch (preset) {
    case 0:
        return QQuaternion::fromAxisAndAngle(0, 0, 1, 180);
    case 1:
        return perspectivePresetRotation();
    case 2:
        return QQuaternion::fromAxisAndAngle(0, 1, 0, 180)
            * QQuaternion::fromAxisAndAngle(0, 0, 1, 180)
            * QQuaternion::fromAxisAndAngle(1, 0, 0, 90)
            * QQuaternion::fromAxisAndAngle(0, 0, 1, 180);
    case 3:
        return QQuaternion::fromAxisAndAngle(1, 0, 0, 90)
            * QQuaternion::fromAxisAndAngle(0, 1, 0, -90);
    case 4:
        return QQuaternion::fromAxisAndAngle(1, 0, 0, 180)
            * QQuaternion::fromAxisAndAngle(0, 0, 1, 180);
    case 5:
        return QQuaternion::fromAxisAndAngle(1, 0, 0, 90);
    case 6:
        return QQuaternion::fromAxisAndAngle(1, 0, 0, 90)
            * QQuaternion::fromAxisAndAngle(0, 1, 0, 90);
    default:
        return QQuaternion::fromAxisAndAngle(0, 0, 1, 180);
    }
}

//=============================================================================================================

bool multiViewPresetIsPerspective(int preset)
{
    return preset == 1;
}

//=============================================================================================================
// SubView factory
//=============================================================================================================

SubView SubView::defaultForIndex(int index)
{
    static constexpr BrainRenderer::ShaderMode kShaderCycle[] = {
        BrainRenderer::Anatomical,
        BrainRenderer::Standard,
        BrainRenderer::Holographic,
    };
    static constexpr int kNumShaders = 3;
    static constexpr int kNumPresets = 7;  // Top..Right

    SubView sv;
    sv.preset      = index % kNumPresets;
    sv.brainShader = kShaderCycle[index % kNumShaders];
    sv.enabled     = true;
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

BrainRenderer::ShaderMode shaderModeFromName(const QString &name)
{
    if (name == "Holographic") return BrainRenderer::Holographic;
    if (name == "Anatomical")  return BrainRenderer::Anatomical;
    return BrainRenderer::Standard;
}

//=============================================================================================================

QString shaderModeName(BrainRenderer::ShaderMode mode)
{
    if (mode == BrainRenderer::Holographic) return QStringLiteral("Holographic");
    if (mode == BrainRenderer::Anatomical)  return QStringLiteral("Anatomical");
    return QStringLiteral("Standard");
}

//=============================================================================================================

BrainSurface::VisualizationMode visualizationModeFromName(const QString &name)
{
    if (name == "Annotation")      return BrainSurface::ModeAnnotation;
    if (name == "Scientific")      return BrainSurface::ModeScientific;
    if (name == "Source Estimate")  return BrainSurface::ModeSourceEstimate;
    return BrainSurface::ModeSurface;
}

//=============================================================================================================

QString visualizationModeName(BrainSurface::VisualizationMode mode)
{
    if (mode == BrainSurface::ModeAnnotation)     return QStringLiteral("Annotation");
    if (mode == BrainSurface::ModeScientific)     return QStringLiteral("Scientific");
    if (mode == BrainSurface::ModeSourceEstimate) return QStringLiteral("Source Estimate");
    return QStringLiteral("Surface");
}

//=============================================================================================================
// Colormap
//=============================================================================================================

QRgb mneAnalyzeColor(double v)
{
    double x = 2.0 * v - 1.0;
    x = std::clamp(x, -1.0, 1.0);

    static constexpr int N = 7;
    static const double pos[N] = { -1.0, -0.90, -0.30, 0.0, 0.30, 0.90, 1.0 };
    static const double rr[N]  = {  0.0,  0.0,   0.5,  0.5, 0.5,  1.0,  1.0 };
    static const double gg[N]  = {  1.0,  0.0,   0.5,  0.5, 0.5,  0.0,  1.0 };
    static const double bb[N]  = {  1.0,  1.0,   0.5,  0.5, 0.5,  0.0,  0.0 };

    int seg = 0;
    for (int i = 0; i < N - 1; ++i) {
        if (x >= pos[i] && x <= pos[i + 1]) {
            seg = i;
            break;
        }
    }
    if (x > pos[N - 1]) seg = N - 2;

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

void SubView::load(const QSettings &settings, const QString &prefix,
                   const QQuaternion &fallbackRotation)
{
    surfaceType = settings.value(prefix + "surfaceType", surfaceType).toString();
    brainShader = shaderModeFromName(settings.value(prefix + "shader", shaderModeName(brainShader)).toString());
    bemShader   = shaderModeFromName(settings.value(prefix + "bemShader", shaderModeName(bemShader)).toString());
    overlayMode = visualizationModeFromName(settings.value(prefix + "overlay", visualizationModeName(overlayMode)).toString());

    visibility.load(settings, prefix + "vis_");

    zoom = settings.value(prefix + "zoom", zoom).toFloat();
    pan = QVector2D(
        settings.value(prefix + "panX", pan.x()).toFloat(),
        settings.value(prefix + "panY", pan.y()).toFloat());
    preset = std::clamp(settings.value(prefix + "preset", preset).toInt(), 0, 6);

    const bool hasQuat = settings.contains(prefix + "perspRotW")
                      && settings.contains(prefix + "perspRotX")
                      && settings.contains(prefix + "perspRotY")
                      && settings.contains(prefix + "perspRotZ");
    if (hasQuat) {
        const float w = settings.value(prefix + "perspRotW", 1.0f).toFloat();
        const float x = settings.value(prefix + "perspRotX", 0.0f).toFloat();
        const float y = settings.value(prefix + "perspRotY", 0.0f).toFloat();
        const float z = settings.value(prefix + "perspRotZ", 0.0f).toFloat();
        perspectiveRotation = QQuaternion(w, x, y, z);
        if (perspectiveRotation.lengthSquared() <= std::numeric_limits<float>::epsilon()) {
            perspectiveRotation = QQuaternion();
        } else {
            perspectiveRotation.normalize();
        }
    } else {
        perspectiveRotation = fallbackRotation;
    }
}

//=============================================================================================================

void SubView::save(QSettings &settings, const QString &prefix) const
{
    settings.setValue(prefix + "surfaceType", surfaceType);
    settings.setValue(prefix + "shader",    shaderModeName(brainShader));
    settings.setValue(prefix + "bemShader", shaderModeName(bemShader));
    settings.setValue(prefix + "overlay",   visualizationModeName(overlayMode));

    visibility.save(settings, prefix + "vis_");

    settings.setValue(prefix + "zoom",   zoom);
    settings.setValue(prefix + "panX",   pan.x());
    settings.setValue(prefix + "panY",   pan.y());
    settings.setValue(prefix + "preset", preset);

    settings.setValue(prefix + "perspRotW", perspectiveRotation.scalar());
    settings.setValue(prefix + "perspRotX", perspectiveRotation.x());
    settings.setValue(prefix + "perspRotY", perspectiveRotation.y());
    settings.setValue(prefix + "perspRotZ", perspectiveRotation.z());
}
