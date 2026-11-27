//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     dataloader.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Loader implementations for FreeSurfer / MNE source-space, BEM and FIFF inputs feeding the mne_viz_qt scene.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dataloader.h"

#include "surfacekeys.h"
#include "../model/items/sensortreeitem.h"
#include "../../../../brain/brainsurface.h"
#include <mne/fiff/fiff_evoked_set.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_dig_point_set.h>
#include <mne/mne/mne_bem.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>
#include <mne/fs/fs_surface.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <format>
#include <memory>
#include <string>
#include <vector>

using namespace mne;

namespace mne::viz::qt
{

//=============================================================================================================
// STATIC METHODS
//=============================================================================================================

DataLoader::SensorLoadResult DataLoader::loadSensors(const std::filesystem::path& fifPath,
                                                     const std::filesystem::path& megHelmetOverridePath)
{
    SensorLoadResult result;

    std::error_code error;
    if (!std::filesystem::exists(fifPath, error))
        return result;

    FiffInfo info;
    FiffDigPointSet digSet;
    FiffStream stream(fifPath);

    if (stream.open()) {
        std::shared_ptr<FiffDirNode> tree = stream.dirtree();
        std::shared_ptr<FiffDirNode> nodeInfo;
        if (stream.read_meas_info(tree, info, nodeInfo)) {
            result.hasInfo = !info.empty();
        }
    }

    if (result.hasInfo) {
        result.info = info;

        // Prepare Device->Head transformation
        QMatrix4x4 devHeadQTrans;
        bool hasDevHead = false;
        if (!info.dev_head_t.empty() &&
            info.dev_head_t.from == FIFFV_COORD_DEVICE &&
            info.dev_head_t.to == FIFFV_COORD_HEAD &&
            !info.dev_head_t.trans.isIdentity()) {
            hasDevHead = true;
            devHeadQTrans = mne::viz::qt::surface_keys::toQMatrix4x4(info.dev_head_t.trans);
        }

        result.devHeadTrans = devHeadQTrans;
        result.hasDevHead = hasDevHead;

        for (const auto& ch : info.chs) {
            if (ch.kind == FIFFV_MEG_CH) {
                QVector3D pos(ch.chpos.r0(0), ch.chpos.r0(1), ch.chpos.r0(2));

                if (hasDevHead) {
                    pos = devHeadQTrans.map(pos);
                }

                auto item = std::make_unique<SensorTreeItem>(QString::fromStdString(ch.ch_name), pos, QColor(100, 100, 100), 0.01f);

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
                    result.megGradItems.push_back(std::move(item));
                } else {
                    result.megMagItems.push_back(std::move(item));
                }
            } else if (ch.kind == FIFFV_EEG_CH) {
                QVector3D pos(ch.chpos.r0(0), ch.chpos.r0(1), ch.chpos.r0(2));
                result.eegItems.push_back(std::make_unique<SensorTreeItem>(QString::fromStdString(ch.ch_name), pos, QColor(0, 200, 220), 0.002f));
            }
        }

        // ── MEG helmet surface ──────────────────────────────────────
        if (!result.megGradItems.empty() || !result.megMagItems.empty()) {
            auto pickHelmetFile = [&info]() -> std::filesystem::path {
                int coilType = -1;
                int nMeg = 0;
                for (const auto& ch : info.chs) {
                    if (ch.kind == FIFFV_MEG_CH) {
                        coilType = ch.chpos.coil_type & 0xFFFF;
                        ++nMeg;
                    }
                }

                std::string fileName = "306m.fif";
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

                return mne::executable_dir() / ".." / "resources" / "general" / "sensorSurfaces" / fileName;
            };

            const auto fileExists = [](const std::filesystem::path& path) {
                std::error_code existsError;
                return !path.empty() && std::filesystem::exists(path, existsError);
            };

            std::filesystem::path helmetPath;
            if (!megHelmetOverridePath.empty()) {
                helmetPath = megHelmetOverridePath;
                if (!fileExists(helmetPath)) {
                    mne::Log::warning("MEG helmet override file not found: {} - falling back to auto selection.", mne::utf8_from_path(helmetPath));
                    helmetPath.clear();
                }
            }

            if (helmetPath.empty()) {
                helmetPath = pickHelmetFile();
#ifdef __EMSCRIPTEN__
                // WASM: the executable directory may not resolve correctly.
                // Try the preloaded virtual FS path directly.
                if (!fileExists(helmetPath)) {
                    std::filesystem::path fn = helmetPath.filename();
                    if (fn.empty())
                        fn = "306m.fif";
                    helmetPath = std::filesystem::path("/resources/general/sensorSurfaces") / fn;
                }
#endif
            }

            if (!fileExists(helmetPath)) {
                const std::filesystem::path fallback = mne::executable_dir() / ".." / "resources" / "general" / "sensorSurfaces" / "306m.fif";
                if (fileExists(fallback)) {
                    helmetPath = fallback;
                }
            }

#ifdef __EMSCRIPTEN__
            // WASM fallback: sensor surfaces are preloaded into the
            // Emscripten virtual FS at /resources/general/sensorSurfaces/.
            // The executable directory may not resolve the /../ correctly.
            if (!fileExists(helmetPath)) {
                std::filesystem::path fn = helmetPath.filename();
                if (fn.empty())
                    fn = "306m.fif";
                const std::filesystem::path wasmFallback = std::filesystem::path("/resources/general/sensorSurfaces") / fn;
                if (fileExists(wasmFallback)) {
                    helmetPath = wasmFallback;
                }
            }
#endif

            if (!fileExists(helmetPath)) {
                mne::Log::warning("MEG helmet surface file not found. Checked: {}", mne::utf8_from_path(helmetPath));
            } else {
                MNEBem helmetBem(helmetPath);
                if (helmetBem.size() > 0) {
                    MNEBemSurface helmetSurf = helmetBem[0];
                    if (helmetSurf.nn.rows() != helmetSurf.rr.rows()) {
                        helmetSurf.nn = mne::FsSurface::compute_normals(Eigen::MatrixX3f(helmetSurf.rr), Eigen::MatrixX3i(helmetSurf.itris));
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
                            const float* d = normalMat.constData();
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
                    helmetSurface->fromBemSurface(helmetSurf, viz::Color{0, 0, 77, 200});
                    helmetSurface->setVisible(true);
                    result.helmetSurface = helmetSurface;
                } else {
                    mne::Log::warning("DataLoader::loadSensors: helmetBem[0] has 0 verts/tris!");
                }
            }
        }
    }

    // ── Digitizer points ────────────────────────────────────────────
    if (result.hasInfo && info.dig.size() > 0) {
        result.digitizerPoints = info.dig;
        result.hasDigitizer = true;
    } else if (!result.hasInfo) {
        if (auto dig = FiffDigPointSet::read(fifPath); dig && !dig->empty()) {
            result.digitizerPoints = std::move(*dig);
            result.hasDigitizer = true;
        }
    }

    return result;
}

//=============================================================================================================

std::shared_ptr<BrainSurface> DataLoader::loadHelmetSurface(
    const std::filesystem::path& helmetFilePath,
    const QMatrix4x4& devHeadTrans,
    bool applyTrans)
{
    std::error_code error;
    if (!std::filesystem::exists(helmetFilePath, error)) {
        mne::Log::warning("DataLoader::loadHelmetSurface: file not found: {}", mne::utf8_from_path(helmetFilePath));
        return nullptr;
    }

    MNEBem helmetBem(helmetFilePath);
    if (helmetBem.size() == 0) {
        mne::Log::warning("DataLoader::loadHelmetSurface: no BEM surfaces in {}", mne::utf8_from_path(helmetFilePath));
        return nullptr;
    }

    MNEBemSurface helmetSurf = helmetBem[0];

    if (helmetSurf.nn.rows() != helmetSurf.rr.rows()) {
        helmetSurf.nn = mne::FsSurface::compute_normals(Eigen::MatrixX3f(helmetSurf.rr), Eigen::MatrixX3i(helmetSurf.itris));
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
            const float* d = normalMat.constData();
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
    surface->fromBemSurface(helmetSurf, viz::Color{0, 0, 77, 200});
    surface->setVisible(true);

    return surface;
}

//=============================================================================================================

InvEcdSet DataLoader::loadDipoles(const std::filesystem::path& dipPath)
{
    InvEcdSet ecdSet = InvEcdSet::read_dipoles_dip(dipPath);
    if (ecdSet.size() == 0) {
        mne::Log::warning("DataLoader: Failed to load dipoles from {}", mne::utf8_from_path(dipPath));
    }
    return ecdSet;
}

//=============================================================================================================

MNESourceSpaces DataLoader::loadSourceSpace(const std::filesystem::path& fwdPath)
{
    auto srcSpace = MNESourceSpaces::read(fwdPath);
    if (!srcSpace) {
        mne::Log::warning("DataLoader: {}", srcSpace.error().what());
        return {};
    }
    return std::move(*srcSpace);
}

//=============================================================================================================

bool DataLoader::loadHeadToMriTransform(const std::filesystem::path& transPath,
                                        FiffCoordTrans& trans)
{
    // A file may hold several transforms (e.g. all-trans.fif: device -> head and MRI -> head)
    const auto read = FiffCoordTrans::read(transPath, FIFFV_COORD_HEAD, FIFFV_COORD_MRI);
    if (!read) {
        mne::Log::warning("DataLoader: {}", read.error().what());
        return false;
    }
    trans = *read;
    return true;
}

//=============================================================================================================

FiffEvoked DataLoader::loadEvoked(const std::filesystem::path& evokedPath, int aveIndex)
{
    std::error_code error;
    if (!std::filesystem::exists(evokedPath, error)) {
        mne::Log::warning("DataLoader: Sensor evoked file not found: {}", mne::utf8_from_path(evokedPath));
        return {};
    }

    FiffEvoked evoked(evokedPath, aveIndex);
    if (evoked.empty()) {
        mne::Log::warning("DataLoader: Failed to read evoked data from {}", mne::utf8_from_path(evokedPath));
    }
    return evoked;
}

//=============================================================================================================

std::vector<std::string> DataLoader::probeEvokedSets(const std::filesystem::path& evokedPath)
{
    std::vector<std::string> result;
    std::error_code error;
    if (!std::filesystem::exists(evokedPath, error)) {
        return result;
    }

    const auto read = FiffEvokedSet::read(evokedPath);
    if (!read) {
        return result;
    }
    const FiffEvokedSet& evokedSet = *read;
    for (int i = 0; i < std::ssize(evokedSet.evoked); ++i) {
        const auto& ev = evokedSet.evoked.at(i);
        result.push_back(std::format("{}: {} ({}, nave={})",
                                     i,
                                     ev.comment.empty() ? std::format("Set {}", i) : ev.comment,
                                     ev.aspectKindToString(),
                                     ev.nave));
    }
    return result;
}

} // namespace mne::viz::qt
