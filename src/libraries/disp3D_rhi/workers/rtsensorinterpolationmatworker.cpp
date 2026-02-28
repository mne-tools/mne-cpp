//=============================================================================================================
/**
 * @file     rtsensorinterpolationmatworker.cpp
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
 * @brief    RtSensorInterpolationMatWorker class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsensorinterpolationmatworker.h"
#include <fwd/fwd_field_map.h>

#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_coord_trans.h>
#include <fwd/fwd_coil_set.h>
#include <fs/surface.h>

#include <QCoreApplication>
#include <QDebug>
#include <QVector3D>
#include <QMatrix4x4>
#include <cmath>

using namespace FIFFLIB;
using namespace DISP3DRHILIB;

//=============================================================================================================
// ANONYMOUS HELPERS
//=============================================================================================================

namespace
{

/**
 * Apply a coordinate transform to a 3-D point using FiffCoordTrans.
 */
Eigen::Vector3f applyTransform(const Eigen::Vector3f &point,
                               const FiffCoordTrans &trans)
{
    if (trans.isEmpty()) return point;
    float r[3] = {point.x(), point.y(), point.z()};
    FiffCoordTrans::apply_trans(r, trans, FIFFV_MOVE);
    return Eigen::Vector3f(r[0], r[1], r[2]);
}

/**
 * Convert a 4×4 Eigen matrix to a QMatrix4x4.
 */
QMatrix4x4 toQMatrix4x4(const Eigen::Matrix4f &m)
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

RtSensorInterpolationMatWorker::RtSensorInterpolationMatWorker(QObject *parent)
    : QObject(parent)
{
}

//=============================================================================================================

void RtSensorInterpolationMatWorker::setEvoked(const FiffEvoked &evoked)
{
    QMutexLocker locker(&m_mutex);
    m_evoked = evoked;
    m_hasEvoked = (evoked.data.rows() > 0 && evoked.data.cols() > 0);
}

//=============================================================================================================

void RtSensorInterpolationMatWorker::setTransform(const FiffCoordTrans &trans,
                                                    bool applySensorTrans)
{
    QMutexLocker locker(&m_mutex);
    m_headToMriTrans = trans;
    m_applySensorTrans = applySensorTrans;
}

//=============================================================================================================

void RtSensorInterpolationMatWorker::setMegFieldMapOnHead(bool onHead)
{
    QMutexLocker locker(&m_mutex);
    m_megOnHead = onHead;
}

//=============================================================================================================

void RtSensorInterpolationMatWorker::setMegSurface(const QString &surfaceKey,
                                                     const Eigen::MatrixX3f &vertices,
                                                     const Eigen::MatrixX3f &normals,
                                                     const Eigen::MatrixX3i &triangles)
{
    QMutexLocker locker(&m_mutex);
    m_megSurfaceKey = surfaceKey;
    m_megVertices = vertices;
    m_megNormals = normals;
    m_megTriangles = triangles;
    m_hasMegSurface = (vertices.rows() > 0);
}

//=============================================================================================================

void RtSensorInterpolationMatWorker::setEegSurface(const QString &surfaceKey,
                                                     const Eigen::MatrixX3f &vertices)
{
    QMutexLocker locker(&m_mutex);
    m_eegSurfaceKey = surfaceKey;
    m_eegVertices = vertices;
    m_hasEegSurface = (vertices.rows() > 0);
}

//=============================================================================================================

void RtSensorInterpolationMatWorker::setBadChannels(const QStringList &bads)
{
    QMutexLocker locker(&m_mutex);
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
    QString megSurfaceKey, eegSurfaceKey;
    Eigen::MatrixX3f megVerts, megNorms, eegVerts;
    Eigen::MatrixX3i megTris;
    bool hasMegSurface, hasEegSurface, hasEvoked;
    QStringList bads;

    {
        QMutexLocker locker(&m_mutex);
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
        qDebug() << "RtSensorInterpolationMatWorker: No evoked data set, skipping.";
        return;
    }

    qDebug() << "RtSensorInterpolationMatWorker: Computing mapping matrices...";

    // ── Build coordinate transforms ────────────────────────────────────
    bool hasDevHead = false;
    QMatrix4x4 devHeadQt;
    if (!evoked.info.dev_head_t.isEmpty() &&
         evoked.info.dev_head_t.from == FIFFV_COORD_DEVICE &&
         evoked.info.dev_head_t.to   == FIFFV_COORD_HEAD &&
        !evoked.info.dev_head_t.trans.isIdentity()) {
        hasDevHead = true;
        devHeadQt = toQMatrix4x4(evoked.info.dev_head_t.trans);
    }

    QMatrix4x4 headToMri;
    if (applySensorTrans && !headToMriTrans.isEmpty()) {
        headToMri = toQMatrix4x4(headToMriTrans.trans);
    }

    // ── Classify channels ──────────────────────────────────────────────
    QList<FiffChInfo> megChs, eegChs;
    QVector<int> megPick, eegPick;

    for (int k = 0; k < evoked.info.chs.size(); ++k) {
        const auto &ch = evoked.info.chs[k];
        if (bads.contains(ch.ch_name)) continue;

        QVector3D pos(ch.chpos.r0(0), ch.chpos.r0(1), ch.chpos.r0(2));

        if (ch.kind == FIFFV_MEG_CH) {
            if (hasDevHead) pos = devHeadQt.map(pos);
            if (applySensorTrans && !headToMriTrans.isEmpty()) pos = headToMri.map(pos);
            megPick.append(k);
            megChs.append(ch);
        } else if (ch.kind == FIFFV_EEG_CH) {
            if (applySensorTrans && !headToMriTrans.isEmpty()) pos = headToMri.map(pos);
            eegPick.append(k);
            eegChs.append(ch);
        }
    }

    // ── Constants (matching MNE-Python) ────────────────────────────────
    constexpr float kIntrad  = 0.06f;
    constexpr float kMegMiss = 1e-4f;
    constexpr float kEegMiss = 1e-3f;
    const Eigen::Vector3f defaultOrigin(0.0f, 0.0f, 0.04f);

    FiffCoordTrans headMri = (applySensorTrans && !headToMriTrans.isEmpty())
        ? headToMriTrans : FiffCoordTrans();
    FiffCoordTrans devHead = (!evoked.info.dev_head_t.isEmpty() &&
                        evoked.info.dev_head_t.from == FIFFV_COORD_DEVICE &&
                        evoked.info.dev_head_t.to   == FIFFV_COORD_HEAD)
        ? evoked.info.dev_head_t : FiffCoordTrans();

    // ── MEG mapping ────────────────────────────────────────────────────
    if (hasMegSurface && !megChs.isEmpty()) {
        Eigen::MatrixX3f norms = megNorms;

        // Recompute normals if missing
        if (norms.rows() != megVerts.rows()) {
            if (megTris.rows() > 0) {
                norms = FSLIB::Surface::compute_normals(megVerts, megTris);
            }
        }

        if (megVerts.rows() > 0 && norms.rows() == megVerts.rows()) {
            const QString coilPath = QCoreApplication::applicationDirPath()
                + "/../resources/general/coilDefinitions/coil_def.dat";
            std::unique_ptr<FWDLIB::FwdCoilSet> templates(
                FWDLIB::FwdCoilSet::read_coil_defs(coilPath));

            if (templates) {
                FiffCoordTrans devToTarget;
                if (megOnHead && !headMri.isEmpty()) {
                    if (!devHead.isEmpty()) {
                        devToTarget = FiffCoordTrans::combine(
                            FIFFV_COORD_DEVICE, FIFFV_COORD_MRI,
                            devHead, headMri);
                    }
                } else if (!devHead.isEmpty()) {
                    devToTarget = devHead;
                }

                Eigen::Vector3f origin = defaultOrigin;
                if (megOnHead && !headMri.isEmpty()) {
                    origin = applyTransform(origin, headMri);
                }

                std::unique_ptr<FWDLIB::FwdCoilSet> coils(templates->create_meg_coils(
                    megChs, megChs.size(), FWD_COIL_ACCURACY_NORMAL, devToTarget));

                if (coils && coils->ncoil > 0) {
                    auto mat = FWDLIB::FwdFieldMap::computeMegMapping(
                        *coils, megVerts, norms, origin, kIntrad, kMegMiss);

                    if (mat && mat->rows() > 0) {
                        qDebug() << "RtSensorInterpolationMatWorker: MEG mapping computed:"
                                 << mat->rows() << "x" << mat->cols();
                        emit newMegMappingAvailable(megSurfaceKey, mat, megPick);
                    }
                }
            }
        }
    }

    // ── EEG mapping ────────────────────────────────────────────────────
    if (hasEegSurface && !eegChs.isEmpty()) {
        if (eegVerts.rows() > 0) {
            Eigen::Vector3f origin = defaultOrigin;
            if (!headMri.isEmpty()) origin = applyTransform(origin, headMri);

            std::unique_ptr<FWDLIB::FwdCoilSet> eegCoils(
                FWDLIB::FwdCoilSet::create_eeg_els(
                    eegChs, eegChs.size(), headMri));

            if (eegCoils && eegCoils->ncoil > 0) {
                auto mat = FWDLIB::FwdFieldMap::computeEegMapping(
                    *eegCoils, eegVerts, origin, kIntrad, kEegMiss);

                if (mat && mat->rows() > 0) {
                    qDebug() << "RtSensorInterpolationMatWorker: EEG mapping computed:"
                             << mat->rows() << "x" << mat->cols();
                    emit newEegMappingAvailable(eegSurfaceKey, mat, eegPick);
                }
            }
        }
    }

    qDebug() << "RtSensorInterpolationMatWorker: Mapping computation complete.";
}
