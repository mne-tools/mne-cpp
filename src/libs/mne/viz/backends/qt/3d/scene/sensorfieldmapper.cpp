//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sensorfieldmapper.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Dense MEG / EEG field-map matrix construction plus iso-contour line-strip generation on the target surface.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sensorfieldmapper.h"
#include "../../qtconversions.h"

#include <mne/base/log.h>

#include "../../../../brain/brainsurface.h"
#include "../../../../brain/rendertypes.h"
#include <mne/base/string_utils.h>
#include <mne/fwd/fwd_field_map.h>
#include <mne/fiff/fiff_ch_info.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_coord_trans.h>
#include <mne/fwd/fwd_coil_set.h>
#include <mne/fs/fs_surface.h>
#include <mne/viz/figures/colormap.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector3D>
#include <QMatrix4x4>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/LU>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <vector>

using namespace mne;

namespace mne::viz::qt
{

//=============================================================================================================
// ANONYMOUS HELPERS
//=============================================================================================================

namespace
{

/**
 * Apply a coordinate transform to a 3-D point using FiffCoordTrans.
 */
Eigen::Vector3f applyTransform(const Eigen::Vector3f& point,
                               const FiffCoordTrans& trans)
{
    if (trans.empty())
        return point;
    float r[3] = {point.x(), point.y(), point.z()};
    FiffCoordTrans::apply_trans(r, trans, FIFFV_MOVE);
    return Eigen::Vector3f(r[0], r[1], r[2]);
}

} // anonymous namespace

//=============================================================================================================
// MEMBER METHODS
//=============================================================================================================

void SensorFieldMapper::setEvoked(const FiffEvoked& evoked)
{
    m_evoked = evoked;
    m_loaded = (m_evoked.nave != -1 && m_evoked.data.rows() > 0);

    // Apply baseline correction if not already applied.
    // This matches MNE-Python's default baseline=(None, 0) which subtracts
    // the mean of the pre-stimulus period (t < 0) from each channel.
    // Without baseline correction the DC offset dominates the mapped field,
    // causing it to appear static ("slightly wobbling") instead of showing
    // the actual temporal evolution of the neural response.
    if (m_loaded && m_evoked.baseline.first == m_evoked.baseline.second) {
        // Find earliest time and t=0 boundaries
        float tmin = m_evoked.times.size() > 0 ? m_evoked.times(0) : 0.0f;
        if (tmin < 0.0f) {
            std::pair<float, float> bl(tmin, 0.0f);
            m_evoked.applyBaselineCorrection(bl);
        }
    }
}

//=============================================================================================================

bool SensorFieldMapper::hasMappingFor(const FiffEvoked& newEvoked) const
{
    // No existing mapping to reuse
    if (!m_loaded || (!m_megMapping && !m_eegMapping))
        return false;

    // Quick check: same number of channels
    if (m_evoked.info.chs.size() != newEvoked.info.chs.size())
        return false;

    // Same channel names in same order
    for (int i = 0; i < std::ssize(m_evoked.info.chs); ++i) {
        if (m_evoked.info.chs[i].ch_name != newEvoked.info.chs[i].ch_name)
            return false;
    }

    // Same bad channels
    if (m_evoked.info.bads != newEvoked.info.bads)
        return false;

    // Same number of SSP projectors
    if (m_evoked.info.projs.size() != newEvoked.info.projs.size())
        return false;

    // Same dev_head transform (sensor positions)
    if (m_evoked.info.dev_head_t.trans != newEvoked.info.dev_head_t.trans)
        return false;

    return true;
}

//=============================================================================================================

std::string SensorFieldMapper::findHeadSurfaceKey(
    const std::map<std::string, std::shared_ptr<BrainSurface>>& surfaces)
{
    if (surfaces.contains("bem_head"))
        return "bem_head";

    std::string fallback;
    for (const auto& [key, surface] : surfaces) {
        if (!key.starts_with("bem_"))
            continue;
        if (surface && surface->tissueType() == BrainSurface::TissueSkin)
            return key;
        if (fallback.empty())
            fallback = key;
    }
    return fallback;
}

//=============================================================================================================

std::string SensorFieldMapper::findHelmetSurfaceKey(
    const std::map<std::string, std::shared_ptr<BrainSurface>>& surfaces)
{
    return surfaces.contains("sens_surface_meg")
        ? std::string("sens_surface_meg")
        : std::string();
}

//=============================================================================================================

float SensorFieldMapper::contourStep(float minVal, float maxVal, int targetTicks)
{
    if (targetTicks <= 0)
        return 0.0f;
    const double range = static_cast<double>(maxVal - minVal);
    if (range <= 0.0)
        return 0.0f;

    const double raw = range / static_cast<double>(targetTicks);
    const double exponent = std::floor(std::log10(raw));
    const double base = std::pow(10.0, exponent);
    const double frac = raw / base;

    double niceFrac = 1.0;
    if (frac <= 1.0)
        niceFrac = 1.0;
    else if (frac <= 2.0)
        niceFrac = 2.0;
    else if (frac <= 5.0)
        niceFrac = 5.0;
    else
        niceFrac = 10.0;

    return static_cast<float>(niceFrac * base);
}

//=============================================================================================================

bool SensorFieldMapper::buildMapping(
    const std::map<std::string, std::shared_ptr<BrainSurface>>& surfaces,
    const FiffCoordTrans& headToMriTrans,
    bool applySensorTrans)
{
    if (!m_loaded || m_evoked.empty())
        return false;

    // ── Reset state ────────────────────────────────────────────────────
    m_megPick.resize(0);
    m_eegPick.resize(0);
    m_megPositions.clear();
    m_eegPositions.clear();
    m_megMapping.reset();
    m_eegMapping.reset();

    // ── Resolve target surfaces ────────────────────────────────────────
    m_megSurfaceKey = m_megOnHead
        ? findHeadSurfaceKey(surfaces)
        : findHelmetSurfaceKey(surfaces);

    if (m_megOnHead && m_megSurfaceKey.empty()) {
        m_megSurfaceKey = findHelmetSurfaceKey(surfaces);
        if (!m_megSurfaceKey.empty())
            mne::Log::warning("SensorFieldMapper: Head surface missing, falling back to helmet.");
    }
    m_eegSurfaceKey = findHeadSurfaceKey(surfaces);

    if (m_megSurfaceKey.empty() && m_eegSurfaceKey.empty()) {
        mne::Log::warning("SensorFieldMapper: No helmet/head surface for field mapping.");
        return false;
    }

    // ── Build coordinate transforms ────────────────────────────────────
    bool hasDevHead = false;
    QMatrix4x4 devHeadQt;
    if (!m_evoked.info.dev_head_t.empty() &&
        m_evoked.info.dev_head_t.from == FIFFV_COORD_DEVICE &&
        m_evoked.info.dev_head_t.to == FIFFV_COORD_HEAD &&
        !m_evoked.info.dev_head_t.trans.isIdentity()) {
        hasDevHead = true;
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                devHeadQt(r, c) = m_evoked.info.dev_head_t.trans(r, c);
    }

    QMatrix4x4 headToMri;
    if (applySensorTrans && !headToMriTrans.empty()) {
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                headToMri(r, c) = headToMriTrans.trans(r, c);
    }

    // ── Classify channels ──────────────────────────────────────────────
    std::vector<FiffChInfo> megChs, eegChs;
    std::vector<std::string> megChNames, eegChNames;

    auto isBad = [this](const std::string& name) {
        return std::ranges::contains(m_evoked.info.bads, name);
    };

    const int nChs = static_cast<int>(m_evoked.info.chs.size());
    m_megPick.resize(nChs); // upper bound
    m_eegPick.resize(nChs);
    int nMeg = 0, nEeg = 0;

    for (int k = 0; k < nChs; ++k) {
        const auto& ch = m_evoked.info.chs[k];
        if (isBad(ch.ch_name))
            continue;

        QVector3D pos(ch.chpos.r0(0), ch.chpos.r0(1), ch.chpos.r0(2));

        if (ch.kind == FIFFV_MEG_CH) {
            if (hasDevHead)
                pos = devHeadQt.map(pos);
            if (applySensorTrans && !headToMriTrans.empty())
                pos = headToMri.map(pos);
            m_megPick(nMeg++) = k;
            m_megPositions.push_back(Eigen::Vector3f(pos.x(), pos.y(), pos.z()));
            megChs.push_back(ch);
            megChNames.push_back(ch.ch_name);
        } else if (ch.kind == FIFFV_EEG_CH) {
            if (applySensorTrans && !headToMriTrans.empty())
                pos = headToMri.map(pos);
            m_eegPick(nEeg++) = k;
            m_eegPositions.push_back(Eigen::Vector3f(pos.x(), pos.y(), pos.z()));
            eegChs.push_back(ch);
            eegChNames.push_back(ch.ch_name);
        }
    }

    m_megPick.conservativeResize(nMeg);
    m_eegPick.conservativeResize(nEeg);

    // ── Constants (matching MNE-Python) ────────────────────────────────
    constexpr float kIntrad = 0.06f;
    constexpr float kMegMiss = 1e-4f;
    constexpr float kEegMiss = 1e-3f;

    // Fit sphere origin to digitisation points (matching MNE-Python's
    // make_field_map with origin='auto').
    const Eigen::Vector3f fittedOrigin = fitSphereOrigin(m_evoked.info);

    FiffCoordTrans headMri = (applySensorTrans && !headToMriTrans.empty())
        ? headToMriTrans
        : FiffCoordTrans();
    FiffCoordTrans devHead = (!m_evoked.info.dev_head_t.empty() &&
                              m_evoked.info.dev_head_t.from == FIFFV_COORD_DEVICE &&
                              m_evoked.info.dev_head_t.to == FIFFV_COORD_HEAD)
        ? m_evoked.info.dev_head_t
        : FiffCoordTrans();

    // ── MEG mapping ────────────────────────────────────────────────────
    if (!m_megSurfaceKey.empty() && surfaces.contains(m_megSurfaceKey) && !megChs.empty()) {
        const BrainSurface& surf = *surfaces.at(m_megSurfaceKey);
        Eigen::MatrixX3f verts = surf.vertexPositions();
        Eigen::MatrixX3f norms = surf.vertexNormals();

        // Recompute normals if missing
        if (norms.rows() != verts.rows()) {
            const std::vector<uint32_t> idx = surf.triangleIndices();
            const int nTris = static_cast<int>(idx.size() / 3);
            if (nTris > 0) {
                Eigen::MatrixX3i tris(nTris, 3);
                for (int t = 0; t < nTris; ++t) {
                    tris(t, 0) = static_cast<int>(idx[t * 3]);
                    tris(t, 1) = static_cast<int>(idx[t * 3 + 1]);
                    tris(t, 2) = static_cast<int>(idx[t * 3 + 2]);
                }
                norms = mne::FsSurface::compute_normals(verts, tris);
            }
        }

        if (verts.rows() > 0 && norms.rows() == verts.rows()) {
            auto templates = mne::FwdCoilSet::read_coil_defs(mne::resource_path("general/coilDefinitions/coil_def.dat"));

            if (templates) {
                FiffCoordTrans devToTarget;
                if (m_megOnHead && !headMri.empty()) {
                    if (!devHead.empty()) {
                        devToTarget = FiffCoordTrans::combine(
                            FIFFV_COORD_DEVICE, FIFFV_COORD_MRI,
                            devHead, headMri);
                    }
                } else if (!devHead.empty()) {
                    devToTarget = devHead;
                }

                Eigen::Vector3f origin = fittedOrigin;
                if (m_megOnHead && !headMri.empty())
                    origin = applyTransform(origin, headMri);

                auto coils = templates->create_meg_coils(
                    megChs, static_cast<int>(megChs.size()), mne::FWD_COIL_ACCURACY_NORMAL, devToTarget);

                if (coils && coils->ncoil() > 0) {
                    m_megMapping = mne::FwdFieldMap::computeMegMapping(
                        *coils, verts, norms, origin,
                        m_evoked.info, megChNames,
                        kIntrad, kMegMiss);
                }
            } else {
                mne::Log::warning("MEG coil definitions not found at {}", mne::utf8_from_path(mne::resource_path("general/coilDefinitions/coil_def.dat")));
            }
        }
    }

    // ── EEG mapping ────────────────────────────────────────────────────
    if (!m_eegSurfaceKey.empty() && surfaces.contains(m_eegSurfaceKey) && !eegChs.empty()) {
        const BrainSurface& surf = *surfaces.at(m_eegSurfaceKey);
        Eigen::MatrixX3f verts = surf.vertexPositions();

        if (verts.rows() > 0) {
            Eigen::Vector3f origin = fittedOrigin;
            if (!headMri.empty())
                origin = applyTransform(origin, headMri);

            auto eegCoils =
                mne::FwdCoilSet::create_eeg_els(
                    eegChs, static_cast<int>(eegChs.size()), headMri);

            if (eegCoils && eegCoils->ncoil() > 0) {
                m_eegMapping = mne::FwdFieldMap::computeEegMapping(
                    *eegCoils, verts, origin,
                    m_evoked.info, eegChNames,
                    kIntrad, kEegMiss);
            }
        }
    }

    computeNormRange();
    return true;
}

//=============================================================================================================

Eigen::Vector3f SensorFieldMapper::fitSphereOrigin(const mne::FiffInfo& info,
                                                   float* radius)
{
    const Eigen::Vector3f fallback(0.0f, 0.0f, 0.04f);

    // ── Gather head-frame digitization points ──────────────────────────
    // MNE-Python's fit_sphere_to_headshape (bem.py) first tries
    // FIFFV_POINT_EXTRA only; if < 4 points, falls back to EXTRA + EEG.
    // Points in the nose/face region (z < 0 && y > 0) are excluded.

    auto gatherPoints = [&](bool includeEeg) -> Eigen::MatrixXd {
        std::vector<Eigen::Vector3d> pts;
        for (const auto& dp : info.dig) {
            if (dp.coord_frame != FIFFV_COORD_HEAD)
                continue;
            if (dp.kind == FIFFV_POINT_EXTRA ||
                (includeEeg && dp.kind == FIFFV_POINT_EEG)) {
                const double x = dp.r[0], y = dp.r[1], z = dp.r[2];
                // Exclude nose / face region
                if (z < 0.0 && y > 0.0)
                    continue;
                pts.push_back(Eigen::Vector3d(x, y, z));
            }
        }

        Eigen::MatrixXd mat(pts.size(), 3);
        for (int i = 0; i < std::ssize(pts); ++i)
            mat.row(i) = pts[i].transpose();
        return mat;
    };

    Eigen::MatrixXd points = gatherPoints(false); // EXTRA only
    if (points.rows() < 4)
        points = gatherPoints(true); // EXTRA + EEG
    if (points.rows() < 4) {
        mne::Log::warning("SensorFieldMapper::fitSphereOrigin: fewer than 4 dig points – falling back to default origin (0, 0, 0.04).");
        if (radius)
            *radius = 0.0f;
        return fallback;
    }

    // ── Linear least-squares sphere fit ────────────────────────────────
    // Expanding  (x-cx)^2 + (y-cy)^2 + (z-cz)^2 = R^2  gives:
    //   2*cx*x + 2*cy*y + 2*cz*z + (R^2 - cx^2 - cy^2 - cz^2) = x^2 + y^2 + z^2
    // which is linear in [cx, cy, cz, D] with D = R^2 - cx^2 - cy^2 - cz^2.
    const int n = static_cast<int>(points.rows());
    Eigen::MatrixXd A(n, 4);
    Eigen::VectorXd b(n);
    for (int i = 0; i < n; ++i) {
        A(i, 0) = 2.0 * points(i, 0);
        A(i, 1) = 2.0 * points(i, 1);
        A(i, 2) = 2.0 * points(i, 2);
        A(i, 3) = 1.0;
        b(i) = points(i, 0) * points(i, 0) + points(i, 1) * points(i, 1) + points(i, 2) * points(i, 2);
    }

    // Solve via normal equations: x = (A^T A)^{-1} A^T b
    // The 4x4 system (A^T A) is tiny and well-conditioned for n >> 4.
    // Use Cramer's rule via Eigen's fixed-size matrix solve.
    Eigen::Matrix4d AtA = A.transpose() * A;
    Eigen::Vector4d Atb = A.transpose() * b;
    // Full-pivot LU for a 4×4 matrix — no extra Eigen module needed.
    Eigen::Vector4d x;
    x = AtA.fullPivLu().solve(Atb);

    const float cx = static_cast<float>(x(0));
    const float cy = static_cast<float>(x(1));
    const float cz = static_cast<float>(x(2));
    const float R = static_cast<float>(
        std::sqrt(x(0) * x(0) + x(1) * x(1) + x(2) * x(2) + x(3)));

    if (radius)
        *radius = R;

    return Eigen::Vector3f(cx, cy, cz);
}

//=============================================================================================================

void SensorFieldMapper::computeNormRange()
{
    m_megVmax = 0.0f;
    m_eegVmax = 0.0f;

    if (!m_loaded || m_evoked.empty())
        return;

    const int nTimes = static_cast<int>(m_evoked.data.cols());

    // ── Helper: find peak-GFP time for a set of channels ───────────────
    // GFP = sqrt(mean(V_i^2)).  We only need the argmax, so comparing
    // the sum-of-squares is sufficient (avoids sqrt).
    auto peakGfpTime = [&](const Eigen::VectorXi& pick) -> int {
        if (pick.size() == 0 || nTimes == 0)
            return 0;
        int best = 0;
        double bestSS = -1.0;
        for (int t = 0; t < nTimes; ++t) {
            double ss = 0.0;
            for (int i = 0; i < pick.size(); ++i) {
                double v = m_evoked.data(pick(i), t);
                ss += v * v;
            }
            if (ss > bestSS) {
                bestSS = ss;
                best = t;
            }
        }
        return best;
    };

    // MEG: anchor vmax to the peak-GFP time point.
    // MNE-Python's plot_field defaults to showing the evoked peak, so its
    // vmax = max(|mapped|) is effectively computed at peak GFP.  Using
    // abs so the symmetric range [-vmax, vmax] always covers both poles.
    if (m_megMapping && m_megMapping->rows() > 0 && m_megPick.size() > 0) {
        const int tPeak = peakGfpTime(m_megPick);
        Eigen::VectorXf meas(m_megPick.size());
        for (int i = 0; i < m_megPick.size(); ++i)
            meas(i) = static_cast<float>(m_evoked.data(m_megPick(i), tPeak));

        Eigen::VectorXf mapped = (*m_megMapping) * meas;
        m_megVmax = mapped.cwiseAbs().maxCoeff();
    }

    // EEG: same strategy
    if (m_eegMapping && m_eegMapping->rows() > 0 && m_eegPick.size() > 0) {
        const int tPeak = peakGfpTime(m_eegPick);
        Eigen::VectorXf meas(m_eegPick.size());
        for (int i = 0; i < m_eegPick.size(); ++i)
            meas(i) = static_cast<float>(m_evoked.data(m_eegPick(i), tPeak));

        Eigen::VectorXf mapped = (*m_eegMapping) * meas;
        m_eegVmax = mapped.cwiseAbs().maxCoeff();
    }

    if (m_megVmax <= 0.0f)
        m_megVmax = 1.0f;
    if (m_eegVmax <= 0.0f)
        m_eegVmax = 1.0f;
}

//=============================================================================================================

void SensorFieldMapper::apply(
    std::map<std::string, std::shared_ptr<BrainSurface>>& surfaces,
    const SubView& singleView,
    const std::vector<SubView>& subViews)
{
    if (!m_loaded || m_evoked.empty())
        return;

    // ── Lambda that maps one modality onto its target surface ───────────
    auto applyMap = [&](const std::string& key,
                        const std::string& contourPrefix,
                        const Eigen::VectorXi& pick,
                        const Eigen::MatrixXf* mat,
                        float globalMaxAbs,
                        bool visible,
                        bool showContours) {
        if (key.empty() || !surfaces.contains(key))
            return;

        auto surface = surfaces[key];
        if (!visible || !mat || pick.size() == 0) {
            surface->setVisualizationMode(mne::viz::ModeSurface);
            updateContourSurfaces(surfaces, contourPrefix, *surface,
                                  std::vector<float>(), 0.0f, false);
            return;
        }
        if (mat->cols() != pick.size()) {
            surface->setVisualizationMode(mne::viz::ModeSurface);
            updateContourSurfaces(surfaces, contourPrefix, *surface,
                                  std::vector<float>(), 0.0f, false);
            return;
        }

        // Assemble measurement vector
        Eigen::VectorXf meas(pick.size());
        for (int i = 0; i < pick.size(); ++i)
            meas(i) = static_cast<float>(m_evoked.data(pick(i), m_timePoint));

        Eigen::VectorXf mapped = (*mat) * meas;

        // Use normalisation range (computed at the anchor time point,
        // matching MNE-Python's plot_field vmax behaviour).
        const float maxAbs = globalMaxAbs;

        // Per-vertex ABGR colours
        std::vector<uint32_t> colors(mapped.size());
        for (int i = 0; i < mapped.size(); ++i) {
            double norm = (mapped(i) / maxAbs) * 0.5 + 0.5;
            norm = std::clamp(norm, 0.0, 1.0);

            const Color rgb = (m_colormap == "MNE") ? mneAnalyzeColor(norm) : map_color(m_colormap, norm);
            colors[i] = packABGR(rgb.r, rgb.g, rgb.b);
        }
        surface->applySourceEstimateColors(colors);

        // Contour lines — 21 levels matching MNE-Python's default
        // (linspace(-vmax, vmax, 21) → step = vmax / 10)
        std::vector<float> values(mapped.size());
        for (int i = 0; i < mapped.size(); ++i)
            values[i] = mapped(i);

        constexpr int nContours = 21;
        float step = (2.0f * maxAbs) / static_cast<float>(nContours - 1);
        updateContourSurfaces(surfaces, contourPrefix, *surface,
                              values, step, showContours);
    };

    // ── Aggregate visibility across all views ──────────────────────────
    bool anyMegField = singleView.visibility.megFieldMap;
    bool anyEegField = singleView.visibility.eegFieldMap;
    bool anyMegContours = singleView.visibility.megFieldContours;
    bool anyEegContours = singleView.visibility.eegFieldContours;
    for (int i = 0; i < std::ssize(subViews); ++i) {
        anyMegField |= subViews[i].visibility.megFieldMap;
        anyEegField |= subViews[i].visibility.eegFieldMap;
        anyMegContours |= subViews[i].visibility.megFieldContours;
        anyEegContours |= subViews[i].visibility.eegFieldContours;
    }

    applyMap(m_megSurfaceKey, m_megContourPrefix,
             m_megPick, m_megMapping.get(),
             m_megVmax,
             anyMegField, anyMegContours);

    applyMap(m_eegSurfaceKey, m_eegContourPrefix,
             m_eegPick, m_eegMapping.get(),
             m_eegVmax,
             anyEegField, anyEegContours);
}

//=============================================================================================================

void SensorFieldMapper::updateContourSurfaces(
    std::map<std::string, std::shared_ptr<BrainSurface>>& surfaces,
    const std::string& prefix,
    const BrainSurface& surface,
    const std::vector<float>& values,
    float step,
    bool visible)
{
    // ── Helper: hide all three contour sets ─────────────────────────────
    auto hideContours = [&]() {
        for (const char* suffix : {"_neg", "_zero", "_pos"}) {
            const auto it = surfaces.find(prefix + suffix);
            if (it != surfaces.end())
                it->second->setVisible(false);
        }
    };

    if (!visible || values.empty() || step <= 0.0f) {
        hideContours();
        return;
    }

    // ── Value range ────────────────────────────────────────────────────
    float minVal = values[0], maxVal = values[0];
    for (int i = 1; i < std::ssize(values); ++i) {
        minVal = std::min(minVal, values[i]);
        maxVal = std::max(maxVal, values[i]);
    }

    // ── Contour levels ─────────────────────────────────────────────────
    std::vector<float> negLevels, posLevels;
    const bool hasZero = (minVal < 0.0f && maxVal > 0.0f);
    for (float lv = -step; lv >= minVal; lv -= step)
        negLevels.push_back(lv);
    for (float lv = step; lv <= maxVal; lv += step)
        posLevels.push_back(lv);

    // ── Segment buffer ─────────────────────────────────────────────────
    struct ContourBuf
    {
        std::vector<Eigen::Vector3f> verts;
        std::vector<Eigen::Vector3f> norms;
        std::vector<Eigen::Vector3i> tris;
    };

    auto addSegment = [](ContourBuf& buf,
                         const QVector3D& p0, const QVector3D& p1,
                         const QVector3D& normal,
                         float halfW, float shift) {
        QVector3D dir = p1 - p0;
        const float len = dir.length();
        if (len < 1e-6f)
            return;
        dir /= len;

        QVector3D binormal = QVector3D::crossProduct(normal, dir);
        if (binormal.length() < 1e-6f)
            binormal = QVector3D::crossProduct(QVector3D(0, 1, 0), dir);
        if (binormal.length() < 1e-6f)
            binormal = QVector3D::crossProduct(QVector3D(1, 0, 0), dir);
        binormal.normalize();

        const QVector3D off = normal * shift;

        auto toEig = [](const QVector3D& v) {
            return Eigen::Vector3f(v.x(), v.y(), v.z());
        };

        // Horizontal quad: width along binormal (visible from above)
        {
            const QVector3D w = binormal * halfW;
            const int base = static_cast<int>(buf.verts.size());
            Eigen::Vector3f n(normal.x(), normal.y(), normal.z());

            buf.verts.push_back(toEig(p0 - w + off));
            buf.verts.push_back(toEig(p0 + w + off));
            buf.verts.push_back(toEig(p1 - w + off));
            buf.verts.push_back(toEig(p1 + w + off));
            buf.norms.push_back(n);
            buf.norms.push_back(n);
            buf.norms.push_back(n);
            buf.norms.push_back(n);
            buf.tris.push_back(Eigen::Vector3i(base, base + 1, base + 2));
            buf.tris.push_back(Eigen::Vector3i(base + 1, base + 3, base + 2));
        }

        // Vertical quad: height along normal (visible from the side)
        {
            const QVector3D h = normal * halfW;
            const int base = static_cast<int>(buf.verts.size());
            Eigen::Vector3f n(binormal.x(), binormal.y(), binormal.z());

            buf.verts.push_back(toEig(p0 - h + off));
            buf.verts.push_back(toEig(p0 + h + off));
            buf.verts.push_back(toEig(p1 - h + off));
            buf.verts.push_back(toEig(p1 + h + off));
            buf.norms.push_back(n);
            buf.norms.push_back(n);
            buf.norms.push_back(n);
            buf.norms.push_back(n);
            buf.tris.push_back(Eigen::Vector3i(base, base + 1, base + 2));
            buf.tris.push_back(Eigen::Vector3i(base + 1, base + 3, base + 2));
        }
    };

    // ── Marching-triangle iso-line extraction ──────────────────────────
    auto buildContours = [&](const std::vector<float>& levels, ContourBuf& buf) {
        const Eigen::MatrixX3f rr = surface.vertexPositions();
        const Eigen::MatrixX3f nn = surface.vertexNormals();
        const std::vector<uint32_t> idx = surface.triangleIndices();
        if (rr.rows() == 0 || nn.rows() == 0 || idx.empty())
            return;

        constexpr float shift = 0.001f;
        constexpr float halfW = 0.0005f;

        for (float level : levels) {
            for (int t = 0; t + 2 < std::ssize(idx); t += 3) {
                const int i0 = idx[t], i1 = idx[t + 1], i2 = idx[t + 2];
                const float v0 = values[i0], v1 = values[i1], v2 = values[i2];

                QVector3D p0(rr(i0, 0), rr(i0, 1), rr(i0, 2));
                QVector3D p1(rr(i1, 0), rr(i1, 1), rr(i1, 2));
                QVector3D p2(rr(i2, 0), rr(i2, 1), rr(i2, 2));

                QVector3D n0(nn(i0, 0), nn(i0, 1), nn(i0, 2));
                QVector3D n1(nn(i1, 0), nn(i1, 1), nn(i1, 2));
                QVector3D n2(nn(i2, 0), nn(i2, 1), nn(i2, 2));
                QVector3D triN = (n0 + n1 + n2).normalized();
                if (triN.length() < 1e-6f)
                    triN = QVector3D::crossProduct(p1 - p0, p2 - p0).normalized();

                std::vector<QVector3D> hits;
                auto checkEdge = [&](const QVector3D& a, const QVector3D& b,
                                     float va, float vb) {
                    if (va == vb)
                        return;
                    float tval = (level - va) / (vb - va);
                    if (tval >= 0.0f && tval < 1.0f)
                        hits.push_back(a + (b - a) * tval);
                };
                checkEdge(p0, p1, v0, v1);
                checkEdge(p1, p2, v1, v2);
                checkEdge(p2, p0, v2, v0);

                if (hits.size() == 2)
                    addSegment(buf, hits[0], hits[1], triN, halfW, shift);
            }
        }
    };

    ContourBuf negBuf, posBuf, zeroBuf;
    buildContours(negLevels, negBuf);
    buildContours(posLevels, posBuf);
    if (hasZero) {
        std::vector<float> zeroLevels = {0.0f};
        buildContours(zeroLevels, zeroBuf);
    }

    // ── Upload contour meshes ──────────────────────────────────────────
    auto updateSurf = [&](const char* suffix,
                          const ContourBuf& buf,
                          const QColor& color,
                          bool show) {
        const std::string key = prefix + suffix;
        if (!show || buf.verts.empty()) {
            if (const auto it = surfaces.find(key); it != surfaces.end())
                it->second->setVisible(false);
            return;
        }

        Eigen::MatrixX3f rr(buf.verts.size(), 3);
        Eigen::MatrixX3f nn(buf.norms.size(), 3);
        Eigen::MatrixX3i tris(buf.tris.size(), 3);
        for (int i = 0; i < std::ssize(buf.verts); ++i) {
            rr.row(i) = buf.verts[i];
            nn.row(i) = buf.norms[i];
        }
        for (int i = 0; i < std::ssize(buf.tris); ++i)
            tris.row(i) = buf.tris[i];

        std::shared_ptr<BrainSurface>& csurf = surfaces[key];
        if (!csurf) {
            csurf = std::make_shared<BrainSurface>();
        }
        csurf->createFromData(rr, nn, tris, toColor(color));
        csurf->setVisible(true);
    };

    updateSurf("_neg", negBuf, QColor(0, 0, 255, 200), visible && !negBuf.verts.empty());
    updateSurf("_zero", zeroBuf, QColor(0, 0, 0, 220), visible && !zeroBuf.verts.empty());
    updateSurf("_pos", posBuf, QColor(255, 0, 0, 200), visible && !posBuf.verts.empty());
}

} // namespace mne::viz::qt
