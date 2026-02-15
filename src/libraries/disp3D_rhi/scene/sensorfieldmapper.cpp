//=============================================================================================================
/**
 * @file     sensorfieldmapper.cpp
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
 * @brief    SensorFieldMapper class implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sensorfieldmapper.h"
#include "helpers/field_map.h"

#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_constants.h>
#include <fiff/c/fiff_coord_trans_old.h>
#include <fwd/fwd_coil_set.h>
#include <fs/surface.h>
#include <disp/plots/helpers/colormap.h>

#include <QCoreApplication>
#include <QVector3D>
#include <QMatrix4x4>
#include <QDebug>
#include <cmath>

using namespace FIFFLIB;

//=============================================================================================================
// ANONYMOUS HELPERS
//=============================================================================================================

namespace
{

/**
 * Convert a modern FiffCoordTrans to the legacy FiffCoordTransOld format
 * required by FwdCoilSet::create_meg_coils / create_eeg_els.
 */
std::unique_ptr<FiffCoordTransOld> toOldTransform(const FiffCoordTrans &trans)
{
    if (trans.isEmpty()) return nullptr;

    auto old = std::make_unique<FiffCoordTransOld>();
    old->from = trans.from;
    old->to   = trans.to;
    old->rot  = trans.trans.block<3, 3>(0, 0);
    old->move = trans.trans.block<3, 1>(0, 3);
    FiffCoordTransOld::add_inverse(old.get());
    return old;
}

/**
 * Apply a legacy coordinate transform to a 3-D point.
 */
Eigen::Vector3f applyOldTransform(const Eigen::Vector3f &point,
                                   const FiffCoordTransOld *trans)
{
    if (!trans) return point;
    float r[3] = {point.x(), point.y(), point.z()};
    FiffCoordTransOld::fiff_coord_trans(r, trans, FIFFV_MOVE);
    return Eigen::Vector3f(r[0], r[1], r[2]);
}

} // anonymous namespace

//=============================================================================================================
// MEMBER METHODS
//=============================================================================================================

void SensorFieldMapper::setEvoked(const FiffEvoked &evoked)
{
    m_evoked = evoked;
    m_loaded = (m_evoked.nave != -1 && m_evoked.data.rows() > 0);
}

//=============================================================================================================

QString SensorFieldMapper::findHeadSurfaceKey(
    const QMap<QString, std::shared_ptr<BrainSurface>> &surfaces)
{
    if (surfaces.contains("bem_head"))
        return QStringLiteral("bem_head");

    QString fallback;
    for (auto it = surfaces.cbegin(); it != surfaces.cend(); ++it) {
        if (!it.key().startsWith("bem_")) continue;
        if (it.value() && it.value()->tissueType() == BrainSurface::TissueSkin)
            return it.key();
        if (fallback.isEmpty())
            fallback = it.key();
    }
    return fallback;
}

//=============================================================================================================

QString SensorFieldMapper::findHelmetSurfaceKey(
    const QMap<QString, std::shared_ptr<BrainSurface>> &surfaces)
{
    return surfaces.contains("sens_surface_meg")
        ? QStringLiteral("sens_surface_meg")
        : QString();
}

//=============================================================================================================

float SensorFieldMapper::contourStep(float minVal, float maxVal, int targetTicks)
{
    if (targetTicks <= 0) return 0.0f;
    const double range = static_cast<double>(maxVal - minVal);
    if (range <= 0.0) return 0.0f;

    const double raw      = range / static_cast<double>(targetTicks);
    const double exponent = std::floor(std::log10(raw));
    const double base     = std::pow(10.0, exponent);
    const double frac     = raw / base;

    double niceFrac = 1.0;
    if      (frac <= 1.0) niceFrac = 1.0;
    else if (frac <= 2.0) niceFrac = 2.0;
    else if (frac <= 5.0) niceFrac = 5.0;
    else                  niceFrac = 10.0;

    return static_cast<float>(niceFrac * base);
}

//=============================================================================================================

bool SensorFieldMapper::buildMapping(
    const QMap<QString, std::shared_ptr<BrainSurface>> &surfaces,
    const FiffCoordTrans &headToMriTrans,
    bool applySensorTrans)
{
    if (!m_loaded || m_evoked.isEmpty()) return false;

    // ── Reset state ────────────────────────────────────────────────────
    m_megPick.clear();
    m_eegPick.clear();
    m_megPositions.clear();
    m_eegPositions.clear();
    m_megMapping.reset();
    m_eegMapping.reset();

    // ── Resolve target surfaces ────────────────────────────────────────
    m_megSurfaceKey = m_megOnHead
        ? findHeadSurfaceKey(surfaces)
        : findHelmetSurfaceKey(surfaces);

    if (m_megOnHead && m_megSurfaceKey.isEmpty()) {
        m_megSurfaceKey = findHelmetSurfaceKey(surfaces);
        if (!m_megSurfaceKey.isEmpty())
            qWarning() << "SensorFieldMapper: Head surface missing, falling back to helmet.";
    }
    m_eegSurfaceKey = findHeadSurfaceKey(surfaces);

    if (m_megSurfaceKey.isEmpty() && m_eegSurfaceKey.isEmpty()) {
        qWarning() << "SensorFieldMapper: No helmet/head surface for field mapping.";
        return false;
    }

    // ── Build coordinate transforms ────────────────────────────────────
    bool hasDevHead = false;
    QMatrix4x4 devHeadQt;
    if (!m_evoked.info.dev_head_t.isEmpty() &&
         m_evoked.info.dev_head_t.from == FIFFV_COORD_DEVICE &&
         m_evoked.info.dev_head_t.to   == FIFFV_COORD_HEAD &&
        !m_evoked.info.dev_head_t.trans.isIdentity()) {
        hasDevHead = true;
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                devHeadQt(r, c) = m_evoked.info.dev_head_t.trans(r, c);
    }

    QMatrix4x4 headToMri;
    if (applySensorTrans && !headToMriTrans.isEmpty()) {
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                headToMri(r, c) = headToMriTrans.trans(r, c);
    }

    // ── Classify channels ──────────────────────────────────────────────
    QList<FiffChInfo> megChs, eegChs;

    auto isBad = [this](const QString &name) {
        return m_evoked.info.bads.contains(name);
    };

    for (int k = 0; k < m_evoked.info.chs.size(); ++k) {
        const auto &ch = m_evoked.info.chs[k];
        if (isBad(ch.ch_name)) continue;

        QVector3D pos(ch.chpos.r0(0), ch.chpos.r0(1), ch.chpos.r0(2));

        if (ch.kind == FIFFV_MEG_CH) {
            if (hasDevHead)                                  pos = devHeadQt.map(pos);
            if (applySensorTrans && !headToMriTrans.isEmpty()) pos = headToMri.map(pos);
            m_megPick.append(k);
            m_megPositions.append(Eigen::Vector3f(pos.x(), pos.y(), pos.z()));
            megChs.append(ch);
        } else if (ch.kind == FIFFV_EEG_CH) {
            if (applySensorTrans && !headToMriTrans.isEmpty()) pos = headToMri.map(pos);
            m_eegPick.append(k);
            m_eegPositions.append(Eigen::Vector3f(pos.x(), pos.y(), pos.z()));
            eegChs.append(ch);
        }
    }

    // ── Constants (matching MNE-Python) ────────────────────────────────
    constexpr float kIntrad  = 0.06f;
    constexpr float kMegMiss = 1e-4f;
    constexpr float kEegMiss = 1e-3f;
    const Eigen::Vector3f defaultOrigin(0.0f, 0.0f, 0.04f);

    auto headMriOld = (applySensorTrans && !headToMriTrans.isEmpty())
        ? toOldTransform(headToMriTrans) : nullptr;
    auto devHeadOld = (!m_evoked.info.dev_head_t.isEmpty() &&
                        m_evoked.info.dev_head_t.from == FIFFV_COORD_DEVICE &&
                        m_evoked.info.dev_head_t.to   == FIFFV_COORD_HEAD)
        ? toOldTransform(m_evoked.info.dev_head_t) : nullptr;

    // ── MEG mapping ────────────────────────────────────────────────────
    if (!m_megSurfaceKey.isEmpty() && surfaces.contains(m_megSurfaceKey) && !megChs.isEmpty()) {
        const BrainSurface &surf = *surfaces[m_megSurfaceKey];
        Eigen::MatrixX3f verts = surf.vertexPositions();
        Eigen::MatrixX3f norms = surf.vertexNormals();

        // Recompute normals if missing
        if (norms.rows() != verts.rows()) {
            const QVector<uint32_t> idx = surf.triangleIndices();
            const int nTris = idx.size() / 3;
            if (nTris > 0) {
                Eigen::MatrixX3i tris(nTris, 3);
                for (int t = 0; t < nTris; ++t) {
                    tris(t, 0) = static_cast<int>(idx[t * 3]);
                    tris(t, 1) = static_cast<int>(idx[t * 3 + 1]);
                    tris(t, 2) = static_cast<int>(idx[t * 3 + 2]);
                }
                norms = FSLIB::Surface::compute_normals(verts, tris);
            }
        }

        if (verts.rows() > 0 && norms.rows() == verts.rows()) {
            const QString coilPath = QCoreApplication::applicationDirPath()
                + "/../resources/general/coilDefinitions/coil_def.dat";
            std::unique_ptr<FWDLIB::FwdCoilSet> templates(
                FWDLIB::FwdCoilSet::read_coil_defs(coilPath));

            if (templates) {
                std::unique_ptr<FiffCoordTransOld> devToTarget;
                if (m_megOnHead && headMriOld) {
                    if (devHeadOld) {
                        devToTarget.reset(FiffCoordTransOld::fiff_combine_transforms(
                            FIFFV_COORD_DEVICE, FIFFV_COORD_MRI,
                            devHeadOld.get(), headMriOld.get()));
                    }
                } else if (devHeadOld) {
                    devToTarget = std::make_unique<FiffCoordTransOld>(*devHeadOld);
                }

                Eigen::Vector3f origin = defaultOrigin;
                if (m_megOnHead && headMriOld)
                    origin = applyOldTransform(origin, headMriOld.get());

                std::unique_ptr<FWDLIB::FwdCoilSet> coils(templates->create_meg_coils(
                    megChs, megChs.size(), FWD_COIL_ACCURACY_NORMAL, devToTarget.get()));

                if (coils && coils->ncoil > 0) {
                    m_megMapping = DISP3DRHILIB::FieldMap::computeMegMapping(
                        *coils, verts, norms, origin, kIntrad, kMegMiss);
                }
            }
        }
    }

    // ── EEG mapping ────────────────────────────────────────────────────
    if (!m_eegSurfaceKey.isEmpty() && surfaces.contains(m_eegSurfaceKey) && !eegChs.isEmpty()) {
        const BrainSurface &surf = *surfaces[m_eegSurfaceKey];
        Eigen::MatrixX3f verts = surf.vertexPositions();

        if (verts.rows() > 0) {
            Eigen::Vector3f origin = defaultOrigin;
            if (headMriOld) origin = applyOldTransform(origin, headMriOld.get());

            std::unique_ptr<FWDLIB::FwdCoilSet> eegCoils(
                FWDLIB::FwdCoilSet::create_eeg_els(
                    eegChs, eegChs.size(), headMriOld.get()));

            if (eegCoils && eegCoils->ncoil > 0) {
                m_eegMapping = DISP3DRHILIB::FieldMap::computeEegMapping(
                    *eegCoils, verts, origin, kIntrad, kEegMiss);
            }
        }
    }

    return true;
}

//=============================================================================================================

void SensorFieldMapper::apply(
    QMap<QString, std::shared_ptr<BrainSurface>> &surfaces,
    const SubView &singleView,
    const SubView (&subViews)[4])
{
    if (!m_loaded || m_evoked.isEmpty()) return;

    // ── Lambda that maps one modality onto its target surface ───────────
    auto applyMap = [&](const QString &key,
                        const QString &contourPrefix,
                        const QVector<int> &pick,
                        const QSharedPointer<Eigen::MatrixXf> &mat,
                        bool visible,
                        bool showContours) {
        if (key.isEmpty() || !surfaces.contains(key)) return;

        auto surface = surfaces[key];
        if (!visible || !mat || pick.isEmpty()) {
            surface->setVisualizationMode(BrainSurface::ModeSurface);
            updateContourSurfaces(surfaces, contourPrefix, *surface,
                                  QVector<float>(), 0.0f, false);
            return;
        }
        if (mat->cols() != pick.size()) {
            surface->setVisualizationMode(BrainSurface::ModeSurface);
            updateContourSurfaces(surfaces, contourPrefix, *surface,
                                  QVector<float>(), 0.0f, false);
            return;
        }

        // Assemble measurement vector
        Eigen::VectorXf meas(pick.size());
        for (int i = 0; i < pick.size(); ++i)
            meas(i) = static_cast<float>(m_evoked.data(pick[i], m_timePoint));

        Eigen::VectorXf mapped = (*mat) * meas;

        // Normalise to symmetric ±maxAbs range
        float maxAbs = 0.0f, minVal = 0.0f, maxVal = 0.0f;
        for (int i = 0; i < mapped.size(); ++i) {
            const float v = mapped(i);
            if (i == 0) { minVal = v; maxVal = v; }
            else { minVal = std::min(minVal, v); maxVal = std::max(maxVal, v); }
            maxAbs = std::max(maxAbs, std::abs(v));
        }
        if (maxAbs <= 0.0f) maxAbs = 1.0f;

        // Per-vertex ABGR colours
        QVector<uint32_t> colors(mapped.size());
        for (int i = 0; i < mapped.size(); ++i) {
            double norm = (mapped(i) / maxAbs) * 0.5 + 0.5;
            norm = qBound(0.0, norm, 1.0);

            QRgb rgb = (m_colormap == "MNE")
                ? mneAnalyzeColor(norm)
                : DISPLIB::ColorMap::valueToColor(norm, m_colormap);

            uint32_t r = qRed(rgb);
            uint32_t g = qGreen(rgb);
            uint32_t b = qBlue(rgb);
            colors[i] = (0xFFu << 24) | (b << 16) | (g << 8) | r;
        }
        surface->applySourceEstimateColors(colors);

        // Contour lines
        QVector<float> values(mapped.size());
        for (int i = 0; i < mapped.size(); ++i)
            values[i] = mapped(i);

        float step = contourStep(-maxAbs, maxAbs, 20);
        updateContourSurfaces(surfaces, contourPrefix, *surface,
                              values, step, showContours);
    };

    // ── Aggregate visibility across all views ──────────────────────────
    bool anyMegField    = singleView.visibility.megFieldMap;
    bool anyEegField    = singleView.visibility.eegFieldMap;
    bool anyMegContours = singleView.visibility.megFieldContours;
    bool anyEegContours = singleView.visibility.eegFieldContours;
    for (int i = 0; i < 4; ++i) {
        anyMegField    |= subViews[i].visibility.megFieldMap;
        anyEegField    |= subViews[i].visibility.eegFieldMap;
        anyMegContours |= subViews[i].visibility.megFieldContours;
        anyEegContours |= subViews[i].visibility.eegFieldContours;
    }

    applyMap(m_megSurfaceKey, m_megContourPrefix,
             m_megPick, m_megMapping,
             anyMegField, anyMegContours);

    applyMap(m_eegSurfaceKey, m_eegContourPrefix,
             m_eegPick, m_eegMapping,
             anyEegField, anyEegContours);
}

//=============================================================================================================

void SensorFieldMapper::updateContourSurfaces(
    QMap<QString, std::shared_ptr<BrainSurface>> &surfaces,
    const QString &prefix,
    const BrainSurface &surface,
    const QVector<float> &values,
    float step,
    bool visible)
{
    // ── Helper: hide all three contour sets ─────────────────────────────
    auto hideContours = [&]() {
        for (const auto &suffix : {QStringLiteral("_neg"),
                                    QStringLiteral("_zero"),
                                    QStringLiteral("_pos")}) {
            const QString key = prefix + suffix;
            if (surfaces.contains(key)) surfaces[key]->setVisible(false);
        }
    };

    if (!visible || values.isEmpty() || step <= 0.0f) {
        hideContours();
        return;
    }

    // ── Value range ────────────────────────────────────────────────────
    float minVal = values[0], maxVal = values[0];
    for (int i = 1; i < values.size(); ++i) {
        minVal = std::min(minVal, values[i]);
        maxVal = std::max(maxVal, values[i]);
    }

    // ── Contour levels ─────────────────────────────────────────────────
    QVector<float> negLevels, posLevels;
    const bool hasZero = (minVal < 0.0f && maxVal > 0.0f);
    for (float lv = -step; lv >= minVal; lv -= step) negLevels.append(lv);
    for (float lv =  step; lv <= maxVal; lv += step) posLevels.append(lv);

    // ── Segment buffer ─────────────────────────────────────────────────
    struct ContourBuf {
        QVector<Eigen::Vector3f> verts;
        QVector<Eigen::Vector3f> norms;
        QVector<Eigen::Vector3i> tris;
    };

    auto addSegment = [](ContourBuf &buf,
                         const QVector3D &p0, const QVector3D &p1,
                         const QVector3D &normal,
                         float halfW, float shift) {
        QVector3D dir = p1 - p0;
        const float len = dir.length();
        if (len < 1e-6f) return;
        dir /= len;

        QVector3D binormal = QVector3D::crossProduct(normal, dir);
        if (binormal.length() < 1e-6f)
            binormal = QVector3D::crossProduct(QVector3D(0, 1, 0), dir);
        if (binormal.length() < 1e-6f)
            binormal = QVector3D::crossProduct(QVector3D(1, 0, 0), dir);
        binormal.normalize();

        const QVector3D off = normal * shift;
        const QVector3D w   = binormal * halfW;
        const int base = buf.verts.size();

        auto toEig = [](const QVector3D &v) {
            return Eigen::Vector3f(v.x(), v.y(), v.z());
        };
        Eigen::Vector3f n(normal.x(), normal.y(), normal.z());

        buf.verts.append(toEig(p0 - w + off));
        buf.verts.append(toEig(p0 + w + off));
        buf.verts.append(toEig(p1 - w + off));
        buf.verts.append(toEig(p1 + w + off));
        buf.norms.append(n); buf.norms.append(n);
        buf.norms.append(n); buf.norms.append(n);
        buf.tris.append(Eigen::Vector3i(base, base + 1, base + 2));
        buf.tris.append(Eigen::Vector3i(base + 1, base + 3, base + 2));
    };

    // ── Marching-triangle iso-line extraction ──────────────────────────
    auto buildContours = [&](const QVector<float> &levels, ContourBuf &buf) {
        const Eigen::MatrixX3f rr = surface.vertexPositions();
        const Eigen::MatrixX3f nn = surface.vertexNormals();
        const QVector<uint32_t> idx = surface.triangleIndices();
        if (rr.rows() == 0 || nn.rows() == 0 || idx.isEmpty()) return;

        constexpr float shift   = 0.001f;
        constexpr float halfW   = 0.0005f;

        for (float level : levels) {
            for (int t = 0; t + 2 < idx.size(); t += 3) {
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

                QVector<QVector3D> hits;
                auto checkEdge = [&](const QVector3D &a, const QVector3D &b,
                                     float va, float vb) {
                    if (va == vb) return;
                    float tval = (level - va) / (vb - va);
                    if (tval >= 0.0f && tval < 1.0f)
                        hits.append(a + (b - a) * tval);
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
        QVector<float> zeroLevels = {0.0f};
        buildContours(zeroLevels, zeroBuf);
    }

    // ── Upload contour meshes ──────────────────────────────────────────
    auto updateSurf = [&](const QString &suffix,
                          const ContourBuf &buf,
                          const QColor &color,
                          bool show) {
        const QString key = prefix + suffix;
        if (!show || buf.verts.isEmpty()) {
            if (surfaces.contains(key)) surfaces[key]->setVisible(false);
            return;
        }

        Eigen::MatrixX3f rr(buf.verts.size(), 3);
        Eigen::MatrixX3f nn(buf.norms.size(), 3);
        Eigen::MatrixX3i tris(buf.tris.size(), 3);
        for (int i = 0; i < buf.verts.size(); ++i) {
            rr.row(i) = buf.verts[i];
            nn.row(i) = buf.norms[i];
        }
        for (int i = 0; i < buf.tris.size(); ++i)
            tris.row(i) = buf.tris[i];

        std::shared_ptr<BrainSurface> csurf;
        if (surfaces.contains(key)) {
            csurf = surfaces[key];
        } else {
            csurf = std::make_shared<BrainSurface>();
            surfaces[key] = csurf;
        }
        csurf->createFromData(rr, nn, tris, color);
        csurf->setVisible(true);
    };

    updateSurf("_neg",  negBuf,  QColor(0,   0, 255, 200), visible && !negBuf.verts.isEmpty());
    updateSurf("_zero", zeroBuf, QColor(0,   0,   0, 220), visible && !zeroBuf.verts.isEmpty());
    updateSurf("_pos",  posBuf,  QColor(255, 0,   0, 200), visible && !posBuf.verts.isEmpty());
}
