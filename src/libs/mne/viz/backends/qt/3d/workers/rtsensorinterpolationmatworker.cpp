//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rtsensorinterpolationmatworker.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Off-thread dense MEG / EEG field-mapping matrix recomputation triggered by configuration changes.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsensorinterpolationmatworker.h"

#include <mne/base/log.h>

#include "../scene/sensorfieldmapper.h"
#include <mne/base/string_utils.h>
#include <mne/fwd/fwd_field_map.h>
#include <mne/fiff/fiff_ch_info.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_coord_trans.h>
#include <mne/fwd/fwd_coil_set.h>
#include <mne/fs/fs_surface.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector3D>
#include <QMatrix4x4>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <vector>

using namespace mne;
using namespace mne::viz;
using namespace mne::viz::qt;

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

/**
 * Convert a 4×4 Eigen matrix to a QMatrix4x4.
 */
QMatrix4x4 toQMatrix4x4(const Eigen::Matrix4f& m)
{
    QMatrix4x4 q;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            q(r, c) = m(r, c);
    return q;
}

} // anonymous namespace

//=============================================================================================================
// MEMBER METHODS
//=============================================================================================================

RtSensorInterpolationMatWorker::RtSensorInterpolationMatWorker(QObject* parent)
: QObject(parent)
{
}

//=============================================================================================================

void RtSensorInterpolationMatWorker::setEvoked(const FiffEvoked& evoked)
{
    std::scoped_lock locker(m_mutex);
    m_evoked = evoked;
    m_hasEvoked = (evoked.data.rows() > 0 && evoked.data.cols() > 0);
}

//=============================================================================================================

void RtSensorInterpolationMatWorker::setTransform(const FiffCoordTrans& trans,
                                                  bool applySensorTrans)
{
    std::scoped_lock locker(m_mutex);
    m_headToMriTrans = trans;
    m_applySensorTrans = applySensorTrans;
}

//=============================================================================================================

void RtSensorInterpolationMatWorker::setMegFieldMapOnHead(bool onHead)
{
    std::scoped_lock locker(m_mutex);
    m_megOnHead = onHead;
}

//=============================================================================================================

void RtSensorInterpolationMatWorker::setMegSurface(const std::string& surfaceKey,
                                                   const Eigen::MatrixX3f& vertices,
                                                   const Eigen::MatrixX3f& normals,
                                                   const Eigen::MatrixX3i& triangles)
{
    std::scoped_lock locker(m_mutex);
    m_megSurfaceKey = surfaceKey;
    m_megVertices = vertices;
    m_megNormals = normals;
    m_megTriangles = triangles;
    m_hasMegSurface = (vertices.rows() > 0);
}

//=============================================================================================================

void RtSensorInterpolationMatWorker::setEegSurface(const std::string& surfaceKey,
                                                   const Eigen::MatrixX3f& vertices)
{
    std::scoped_lock locker(m_mutex);
    m_eegSurfaceKey = surfaceKey;
    m_eegVertices = vertices;
    m_hasEegSurface = (vertices.rows() > 0);
}

//=============================================================================================================

void RtSensorInterpolationMatWorker::setBadChannels(const std::vector<std::string>& bads)
{
    std::scoped_lock locker(m_mutex);
    m_bads = bads;
}

//=============================================================================================================

void RtSensorInterpolationMatWorker::computeMapping()
{
    // ── Snapshot parameters under lock ─────────────────────────────────
    FiffEvoked evoked;
    FiffCoordTrans headToMriTrans;
    bool applySensorTrans;
    bool megOnHead;
    std::string megSurfaceKey, eegSurfaceKey;
    Eigen::MatrixX3f megVerts, megNorms, eegVerts;
    Eigen::MatrixX3i megTris;
    bool hasMegSurface, hasEegSurface, hasEvoked;
    std::vector<std::string> bads;

    {
        std::scoped_lock locker(m_mutex);
        evoked = m_evoked;
        hasEvoked = m_hasEvoked;
        headToMriTrans = m_headToMriTrans;
        applySensorTrans = m_applySensorTrans;
        megOnHead = m_megOnHead;
        megSurfaceKey = m_megSurfaceKey;
        eegSurfaceKey = m_eegSurfaceKey;
        megVerts = m_megVertices;
        megNorms = m_megNormals;
        megTris = m_megTriangles;
        hasMegSurface = m_hasMegSurface;
        eegVerts = m_eegVertices;
        hasEegSurface = m_hasEegSurface;
        bads = m_bads;
    }

    if (!hasEvoked) {
        mne::Log::debug("RtSensorInterpolationMatWorker: No evoked data set, skipping.");
        return;
    }

    mne::Log::debug("RtSensorInterpolationMatWorker: Computing mapping matrices...");

    // ── Build coordinate transforms ────────────────────────────────────
    bool hasDevHead = false;
    QMatrix4x4 devHeadQt;
    if (!evoked.info.dev_head_t.empty() &&
        evoked.info.dev_head_t.from == FIFFV_COORD_DEVICE &&
        evoked.info.dev_head_t.to == FIFFV_COORD_HEAD &&
        !evoked.info.dev_head_t.trans.isIdentity()) {
        hasDevHead = true;
        devHeadQt = toQMatrix4x4(evoked.info.dev_head_t.trans);
    }

    QMatrix4x4 headToMri;
    if (applySensorTrans && !headToMriTrans.empty()) {
        headToMri = toQMatrix4x4(headToMriTrans.trans);
    }

    // ── Classify channels ──────────────────────────────────────────────
    std::vector<FiffChInfo> megChs, eegChs;
    std::vector<int> megPick, eegPick;

    for (int k = 0; k < std::ssize(evoked.info.chs); ++k) {
        const auto& ch = evoked.info.chs[k];
        if (std::ranges::contains(bads, ch.ch_name) || std::ranges::contains(evoked.info.bads, ch.ch_name))
            continue;

        QVector3D pos(ch.chpos.r0(0), ch.chpos.r0(1), ch.chpos.r0(2));

        if (ch.kind == FIFFV_MEG_CH) {
            if (hasDevHead)
                pos = devHeadQt.map(pos);
            if (applySensorTrans && !headToMriTrans.empty())
                pos = headToMri.map(pos);
            megPick.push_back(k);
            megChs.push_back(ch);
        } else if (ch.kind == FIFFV_EEG_CH) {
            if (applySensorTrans && !headToMriTrans.empty())
                pos = headToMri.map(pos);
            eegPick.push_back(k);
            eegChs.push_back(ch);
        }
    }

    // ── Constants (matching MNE-Python) ────────────────────────────────
    constexpr float kIntrad = 0.06f;
    constexpr float kMegMiss = 1e-4f;
    constexpr float kEegMiss = 1e-3f;
    // origin="auto" of MNE-Python's make_field_map, as in SensorFieldMapper::buildMapping()
    const Eigen::Vector3f fittedOrigin = SensorFieldMapper::fitSphereOrigin(evoked.info);

    FiffCoordTrans headMri = (applySensorTrans && !headToMriTrans.empty())
        ? headToMriTrans
        : FiffCoordTrans();
    FiffCoordTrans devHead = (!evoked.info.dev_head_t.empty() &&
                              evoked.info.dev_head_t.from == FIFFV_COORD_DEVICE &&
                              evoked.info.dev_head_t.to == FIFFV_COORD_HEAD)
        ? evoked.info.dev_head_t
        : FiffCoordTrans();

    // ── MEG mapping ────────────────────────────────────────────────────
    if (hasMegSurface && !megChs.empty()) {
        Eigen::MatrixX3f norms = megNorms;

        // Recompute normals if missing
        if (norms.rows() != megVerts.rows()) {
            if (megTris.rows() > 0) {
                norms = mne::FsSurface::compute_normals(megVerts, megTris);
            }
        }

        if (megVerts.rows() > 0 && norms.rows() == megVerts.rows()) {
            auto templates = mne::FwdCoilSet::read_coil_defs(mne::resource_path("general/coilDefinitions/coil_def.dat"));

            if (templates) {
                FiffCoordTrans devToTarget;
                if (megOnHead && !headMri.empty()) {
                    if (!devHead.empty()) {
                        devToTarget = FiffCoordTrans::combine(
                            FIFFV_COORD_DEVICE, FIFFV_COORD_MRI,
                            devHead, headMri);
                    }
                } else if (!devHead.empty()) {
                    devToTarget = devHead;
                }

                Eigen::Vector3f origin = fittedOrigin;
                if (megOnHead && !headMri.empty()) {
                    origin = applyTransform(origin, headMri);
                }

                auto coils = templates->create_meg_coils(
                    megChs, static_cast<int>(megChs.size()), mne::FWD_COIL_ACCURACY_NORMAL, devToTarget);

                if (coils && coils->ncoil() > 0) {
                    auto mat = mne::FwdFieldMap::computeMegMapping(
                        *coils, megVerts, norms, origin, kIntrad, kMegMiss);

                    if (mat && mat->rows() > 0) {
                        mne::Log::debug("RtSensorInterpolationMatWorker: MEG mapping computed: {} x {}", mat->rows(), mat->cols());
                        emit newMegMappingAvailable(megSurfaceKey,
                                                    std::shared_ptr<Eigen::MatrixXf>(std::move(mat)), megPick);
                    }
                }
            }
        }
    }

    // ── EEG mapping ────────────────────────────────────────────────────
    if (hasEegSurface && !eegChs.empty()) {
        if (eegVerts.rows() > 0) {
            Eigen::Vector3f origin = fittedOrigin;
            if (!headMri.empty())
                origin = applyTransform(origin, headMri);

            auto eegCoils =
                mne::FwdCoilSet::create_eeg_els(
                    eegChs, static_cast<int>(eegChs.size()), headMri);

            if (eegCoils && eegCoils->ncoil() > 0) {
                auto mat = mne::FwdFieldMap::computeEegMapping(
                    *eegCoils, eegVerts, origin, kIntrad, kEegMiss);

                if (mat && mat->rows() > 0) {
                    mne::Log::debug("RtSensorInterpolationMatWorker: EEG mapping computed: {} x {}", mat->rows(), mat->cols());
                    emit newEegMappingAvailable(eegSurfaceKey,
                                                std::shared_ptr<Eigen::MatrixXf>(std::move(mat)), eegPick);
                }
            }
        }
    }

    mne::Log::debug("RtSensorInterpolationMatWorker: Mapping computation complete.");
}
