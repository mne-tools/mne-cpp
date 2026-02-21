//=============================================================================================================
/**
 * @file     dataloader.cpp
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
 * @brief    DataLoader implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dataloader.h"
#include "core/surfacekeys.h"
#include "model/items/sensortreeitem.h"
#include "renderable/brainsurface.h"

#include <QFile>
#include <QDebug>
#include <QCoreApplication>

#include <Eigen/Dense>
#include <fiff/fiff.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_dig_point_set.h>
#include <mne/mne_bem.h>
#include <fs/surface.h>

using namespace FIFFLIB;
using namespace MNELIB;
using namespace INVERSELIB;

//=============================================================================================================
// STATIC METHODS
//=============================================================================================================

DataLoader::SensorLoadResult DataLoader::loadSensors(const QString &fifPath,
                                                     const QString &megHelmetOverridePath)
{
    SensorLoadResult result;

    QFile file(fifPath);
    if (!file.exists()) return result;

    FiffInfo info;
    FiffDigPointSet digSet;
    FiffStream::SPtr stream(new FiffStream(&file));

    if (stream->open()) {
        FiffDirNode::SPtr tree = stream->dirtree();
        FiffDirNode::SPtr nodeInfo;
        if (stream->read_meas_info(tree, info, nodeInfo)) {
            result.hasInfo = !info.isEmpty();
        }
    }

    if (result.hasInfo) {
        result.info = info;

        // Prepare Device->Head transformation
        QMatrix4x4 devHeadQTrans;
        bool hasDevHead = false;
        if (!info.dev_head_t.isEmpty() &&
            info.dev_head_t.from == FIFFV_COORD_DEVICE &&
            info.dev_head_t.to == FIFFV_COORD_HEAD &&
            !info.dev_head_t.trans.isIdentity()) {
            hasDevHead = true;
            devHeadQTrans = SURFACEKEYS::toQMatrix4x4(info.dev_head_t.trans);
        }

        result.devHeadTrans = devHeadQTrans;
        result.hasDevHead   = hasDevHead;

        for (const auto &ch : info.chs) {
            if (ch.kind == FIFFV_MEG_CH) {
                QVector3D pos(ch.chpos.r0(0), ch.chpos.r0(1), ch.chpos.r0(2));

                if (hasDevHead) {
                    pos = devHeadQTrans.map(pos);
                }

                auto *item = new SensorTreeItem(ch.ch_name, pos, QColor(100, 100, 100), 0.01f);

                // Store coil orientation
                QMatrix4x4 orient;
                for (int r = 0; r < 3; ++r)
                    for (int c = 0; c < 3; ++c)
                        orient(r, c) = ch.coil_trans(r, c);
                if (hasDevHead) {
                    QMatrix4x4 devHeadRot;
                    for (int r = 0; r < 3; ++r)
                        for (int c = 0; c < 3; ++c)
                            devHeadRot(r, c) = devHeadQTrans(r, c);
                    orient = devHeadRot * orient;
                }
                item->setOrientation(orient);

                if (ch.unit == FIFF_UNIT_T_M) {
                    result.megGradItems.append(item);
                } else {
                    result.megMagItems.append(item);
                }
            } else if (ch.kind == FIFFV_EEG_CH) {
                QVector3D pos(ch.chpos.r0(0), ch.chpos.r0(1), ch.chpos.r0(2));
                result.eegItems.append(new SensorTreeItem(ch.ch_name, pos, QColor(0, 200, 220), 0.002f));
            }
        }

        // ── MEG helmet surface ──────────────────────────────────────
        if (!result.megGradItems.isEmpty() || !result.megMagItems.isEmpty()) {
            auto pickHelmetFile = [&info]() -> QString {
                int coilType = -1;
                int nMeg = 0;
                for (const auto &ch : info.chs) {
                    if (ch.kind == FIFFV_MEG_CH) {
                        coilType = ch.chpos.coil_type & 0xFFFF;
                        ++nMeg;
                    }
                }

                QString fileName = "306m.fif";
                if (coilType == FIFFV_COIL_BABY_GRAD) {
                    fileName = "BabySQUID.fif";
                } else if (coilType == FIFFV_COIL_NM_122) {
                    fileName = "122m.fif";
                } else if (coilType == FIFFV_COIL_CTF_GRAD) {
                    fileName = "CTF_275.fif";
                } else if (coilType == FIFFV_COIL_KIT_GRAD) {
                    fileName = "KIT.fif";
                } else if (coilType == FIFFV_COIL_MAGNES_MAG || coilType == FIFFV_COIL_MAGNES_GRAD) {
                    fileName = (nMeg > 150) ? "Magnes_3600wh.fif" : "Magnes_2500wh.fif";
                } else if (coilType / 1000 == 3) {
                    fileName = "306m.fif";
                }

                return QCoreApplication::applicationDirPath()
                    + "/../resources/general/sensorSurfaces/" + fileName;
            };

            QString helmetPath;
            if (!megHelmetOverridePath.isEmpty()) {
                helmetPath = megHelmetOverridePath;
                if (!QFile::exists(helmetPath)) {
                    qWarning() << "MEG helmet override file not found:" << helmetPath
                               << "- falling back to auto selection.";
                    helmetPath.clear();
                }
            }

            if (helmetPath.isEmpty()) {
                helmetPath = pickHelmetFile();
            }

            if (!QFile::exists(helmetPath)) {
                QString fallback = QCoreApplication::applicationDirPath()
                    + "/../resources/general/sensorSurfaces/306m.fif";
                if (QFile::exists(fallback)) {
                    helmetPath = fallback;
                }
            }

            if (!QFile::exists(helmetPath)) {
                qWarning() << "MEG helmet surface file not found. Checked:" << helmetPath;
            }

            if (QFile::exists(helmetPath)) {
                QFile helmetFile(helmetPath);
                MNEBem helmetBem(helmetFile);
                if (helmetBem.size() > 0) {
                    MNEBemSurface helmetSurf = helmetBem[0];

                    if (helmetSurf.nn.rows() != helmetSurf.rr.rows()) {
                        helmetSurf.nn = FSLIB::Surface::compute_normals(helmetSurf.rr, helmetSurf.tris);
                    }

                    if (hasDevHead) {
                        QMatrix3x3 normalMat = devHeadQTrans.normalMatrix();
                        for (int i = 0; i < helmetSurf.rr.rows(); ++i) {
                            QVector3D pos(helmetSurf.rr(i, 0), helmetSurf.rr(i, 1), helmetSurf.rr(i, 2));
                            pos = devHeadQTrans.map(pos);
                            helmetSurf.rr(i, 0) = pos.x();
                            helmetSurf.rr(i, 1) = pos.y();
                            helmetSurf.rr(i, 2) = pos.z();

                            QVector3D nn(helmetSurf.nn(i, 0), helmetSurf.nn(i, 1), helmetSurf.nn(i, 2));
                            const float *d = normalMat.constData();
                            float nx = d[0] * nn.x() + d[3] * nn.y() + d[6] * nn.z();
                            float ny = d[1] * nn.x() + d[4] * nn.y() + d[7] * nn.z();
                            float nz = d[2] * nn.x() + d[5] * nn.y() + d[8] * nn.z();
                            QVector3D n = QVector3D(nx, ny, nz).normalized();
                            helmetSurf.nn(i, 0) = n.x();
                            helmetSurf.nn(i, 1) = n.y();
                            helmetSurf.nn(i, 2) = n.z();
                        }
                    }

                    auto helmetSurface = std::make_shared<BrainSurface>();
                    helmetSurface->fromBemSurface(helmetSurf, QColor(0, 0, 77, 200));
                    helmetSurface->setVisible(true);
                    result.helmetSurface = helmetSurface;
                }
            }
        }
    }

    // ── Digitizer points ────────────────────────────────────────────
    if (result.hasInfo && info.dig.size() > 0) {
        result.digitizerPoints = info.dig;
        result.hasDigitizer = true;
    } else if (!result.hasInfo) {
        file.reset();
        digSet = FiffDigPointSet(file);
        if (digSet.size() > 0) {
            for (int i = 0; i < digSet.size(); ++i) {
                result.digitizerPoints.append(digSet[i]);
            }
            result.hasDigitizer = true;
        }
    }

    return result;
}

//=============================================================================================================

std::shared_ptr<BrainSurface> DataLoader::loadHelmetSurface(
    const QString &helmetFilePath,
    const QMatrix4x4 &devHeadTrans,
    bool applyTrans)
{
    if (!QFile::exists(helmetFilePath)) {
        qWarning() << "DataLoader::loadHelmetSurface: file not found:" << helmetFilePath;
        return nullptr;
    }

    QFile helmetFile(helmetFilePath);
    MNEBem helmetBem(helmetFile);
    if (helmetBem.size() == 0) {
        qWarning() << "DataLoader::loadHelmetSurface: no BEM surfaces in" << helmetFilePath;
        return nullptr;
    }

    MNEBemSurface helmetSurf = helmetBem[0];

    if (helmetSurf.nn.rows() != helmetSurf.rr.rows()) {
        helmetSurf.nn = FSLIB::Surface::compute_normals(helmetSurf.rr, helmetSurf.tris);
    }

    if (applyTrans) {
        QMatrix3x3 normalMat = devHeadTrans.normalMatrix();
        for (int i = 0; i < helmetSurf.rr.rows(); ++i) {
            QVector3D pos(helmetSurf.rr(i, 0), helmetSurf.rr(i, 1), helmetSurf.rr(i, 2));
            pos = devHeadTrans.map(pos);
            helmetSurf.rr(i, 0) = pos.x();
            helmetSurf.rr(i, 1) = pos.y();
            helmetSurf.rr(i, 2) = pos.z();

            QVector3D nn(helmetSurf.nn(i, 0), helmetSurf.nn(i, 1), helmetSurf.nn(i, 2));
            const float *d = normalMat.constData();
            float nx = d[0] * nn.x() + d[3] * nn.y() + d[6] * nn.z();
            float ny = d[1] * nn.x() + d[4] * nn.y() + d[7] * nn.z();
            float nz = d[2] * nn.x() + d[5] * nn.y() + d[8] * nn.z();
            QVector3D n = QVector3D(nx, ny, nz).normalized();
            helmetSurf.nn(i, 0) = n.x();
            helmetSurf.nn(i, 1) = n.y();
            helmetSurf.nn(i, 2) = n.z();
        }
    }

    auto surface = std::make_shared<BrainSurface>();
    surface->fromBemSurface(helmetSurf, QColor(0, 0, 77, 200));
    surface->setVisible(true);

    qDebug() << "DataLoader: Loaded helmet surface from" << helmetFilePath
             << "(" << helmetSurf.rr.rows() << "vertices)";

    return surface;
}

//=============================================================================================================

ECDSet DataLoader::loadDipoles(const QString &dipPath)
{
    ECDSet ecdSet = ECDSet::read_dipoles_dip(dipPath);
    if (ecdSet.size() == 0) {
        qWarning() << "DataLoader: Failed to load dipoles from" << dipPath;
    }
    return ecdSet;
}

//=============================================================================================================

MNESourceSpace DataLoader::loadSourceSpace(const QString &fwdPath)
{
    QFile file(fwdPath);
    if (!file.exists()) {
        qWarning() << "DataLoader: Source space file not found:" << fwdPath;
        return {};
    }

    MNESourceSpace srcSpace;
    FiffStream::SPtr stream(new FiffStream(&file));
    if (!stream->open()) {
        qWarning() << "DataLoader: Failed to open FIF stream for source space";
        return {};
    }

    if (!MNESourceSpace::readFromStream(stream, true, srcSpace)) {
        qWarning() << "DataLoader: Failed to read source space from" << fwdPath;
        return {};
    }

    if (srcSpace.isEmpty()) {
        qWarning() << "DataLoader: Source space is empty";
        return {};
    }

    qDebug() << "DataLoader: Loaded source space with" << srcSpace.size() << "hemispheres";
    for (int h = 0; h < srcSpace.size(); ++h) {
        qDebug() << "  Hemi" << h << ": nuse =" << srcSpace[h].nuse << "np =" << srcSpace[h].np;
    }

    return srcSpace;
}

//=============================================================================================================

bool DataLoader::loadHeadToMriTransform(const QString &transPath,
                                        FiffCoordTrans &trans)
{
    QFile file(transPath);

    FiffCoordTrans raw;
    if (!FiffCoordTrans::read(file, raw)) {
        qWarning() << "DataLoader: Failed to load transformation from" << transPath;
        return false;
    }
    file.close();

    if (raw.from == FIFFV_COORD_HEAD && raw.to == FIFFV_COORD_MRI) {
        trans = raw;
    } else if (raw.from == FIFFV_COORD_MRI && raw.to == FIFFV_COORD_HEAD) {
        // Invert: MRI->Head becomes Head->MRI
        trans.from = raw.to;
        trans.to = raw.from;
        trans.trans = raw.trans.inverse();
        trans.invtrans = raw.trans;
    } else {
        qWarning() << "DataLoader: Loaded transformation is not Head<->MRI (from"
                    << raw.from << "to" << raw.to << "). Using as is.";
        trans = raw;
    }

    return true;
}

//=============================================================================================================

FiffEvoked DataLoader::loadEvoked(const QString &evokedPath, int aveIndex)
{
    QFile file(evokedPath);
    if (!file.exists()) {
        qWarning() << "DataLoader: Sensor evoked file not found:" << evokedPath;
        return {};
    }

    FiffEvoked evoked(file, aveIndex);
    if (evoked.isEmpty()) {
        qWarning() << "DataLoader: Failed to read evoked data from" << evokedPath;
    }
    return evoked;
}

//=============================================================================================================

QStringList DataLoader::probeEvokedSets(const QString &evokedPath)
{
    QStringList result;
    QFile file(evokedPath);
    if (!file.exists()) {
        return result;
    }

    FiffEvokedSet evokedSet(file);
    for (int i = 0; i < evokedSet.evoked.size(); ++i) {
        const auto &ev = evokedSet.evoked.at(i);
        QString label = QString("%1: %2 (%3, nave=%4)")
            .arg(i)
            .arg(ev.comment.isEmpty() ? QStringLiteral("Set %1").arg(i) : ev.comment)
            .arg(ev.aspectKindToString())
            .arg(ev.nave);
        result.append(label);
    }
    return result;
}
