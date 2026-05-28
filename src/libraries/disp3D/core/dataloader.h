//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file dataloader.h
 * @since 2026
 * @date  March 2026
 * @brief Static helpers for loading FreeSurfer / MNE source-space and BEM data into the disp3D model tree.
 *
 * DataLoader is a thin facade over @ref FSLIB and @ref MNELIB readers
 * that turns raw files (FsSurface, FsAnnotation, MNESourceSpaces,
 * MNEBemSurface, FIFF digitizer / sensor blocks) into the in-memory
 * objects that @ref BrainTreeModel and the @c renderable/ layer
 * expect. It exists so GUI code can populate the 3-D scene without
 * knowing the FreeSurfer / MNE file-format conventions (subject
 * directory layout, hemisphere suffixes, surface-RAS vs head
 * coordinate frames, ...).
 *
 * All methods are stateless and may be called from worker threads;
 * they return shared pointers to the produced data structures and do
 * not touch the Qt RHI or any GUI state.
 */

#ifndef DATALOADER_H
#define DATALOADER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

#include <QStringList>
#include <QList>
#include <QStandardItem>
#include <QVector3D>
#include <QMatrix4x4>
#include <memory>

#include <fiff/fiff_info.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_dig_point.h>
#include <mne/mne_source_spaces.h>
#include <inv/dipole_fit/inv_ecd_set.h>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class BrainSurface;
class SensorTreeItem;

//=============================================================================================================
/**
 * @class DataLoader
 * @brief Pure I/O helpers that load MNE data files and return results.
 *
 * These static methods have zero coupling to the widget layer.  BrainView
 * calls them to obtain data and then wires the results into its own model /
 * surface map / member state.
 */
class DISP3DSHARED_EXPORT DataLoader
{
public:
    DataLoader() = delete; // all-static class

    // ── Result structs ────────────────────────────────────────────────

    /**
     * @brief Return value bundling loaded sensor geometry, labels, and channel-to-sensor mapping.
     *
     * Result of reading a FIF file for sensor information.
     */
    struct SensorLoadResult
    {
        bool                        hasInfo     = false;
        bool                        hasDigitizer = false;

        FIFFLIB::FiffInfo           info;          ///< Channel / dig info
        QList<QStandardItem*>       megGradItems;  ///< Ownership passes to caller
        QList<QStandardItem*>       megMagItems;
        QList<QStandardItem*>       eegItems;
        QList<FIFFLIB::FiffDigPoint> digitizerPoints;

        std::shared_ptr<BrainSurface> helmetSurface; ///< May be null

        QMatrix4x4                  devHeadTrans;  ///< Device→Head transform (identity if absent)
        bool                        hasDevHead = false; ///< Whether a valid dev→head transform was found
    };

    // ── Static I/O methods ────────────────────────────────────────────

    /**
     * Load sensor channels, digitizer points and MEG helmet from a FIF file.
     *
     * @param[in] fifPath               Path to the FIF file.
     * @param[in] megHelmetOverridePath  Optional override for the helmet surface file.
     * @return Populated SensorLoadResult.
     */
    static SensorLoadResult loadSensors(const QString &fifPath,
                                        const QString &megHelmetOverridePath = {});

    /**
     * Load a standalone MEG helmet surface from a BEM FIF file.
     *
     * @param[in] helmetFilePath   Path to the helmet BEM FIF file.
     * @param[in] devHeadTrans     Device-to-Head transformation matrix (identity if none).
     * @param[in] applyTrans       Whether to apply the dev→head transform.
     * @return Shared pointer to the loaded BrainSurface, or nullptr on failure.
     */
    static std::shared_ptr<BrainSurface> loadHelmetSurface(
        const QString &helmetFilePath,
        const QMatrix4x4 &devHeadTrans = QMatrix4x4(),
        bool applyTrans = false);

    /**
     * Load dipole set from a .dip file.
     *
     * @param[in] dipPath  Path to the dipole file.
     * @return The loaded InvEcdSet (may be empty on failure).
     */
    static INVLIB::InvEcdSet loadDipoles(const QString &dipPath);

    /**
     * Load source space from a forward-solution FIF file.
     *
     * @param[in] fwdPath  Path to the FIF file with source space.
     * @return The loaded source space (may be empty on failure).
     */
    static MNELIB::MNESourceSpaces loadSourceSpace(const QString &fwdPath);

    /**
     * Load a coordinate transformation from a FIF file and normalise it
     * to Head → MRI orientation.
     *
     * @param[in] transPath  Path to the FIF file.
     * @param[out] trans     The resulting transform (Head → MRI).
     * @return true on success.
     */
    static bool loadHeadToMriTransform(const QString &transPath,
                                       FIFFLIB::FiffCoordTrans &trans);

    /**
     * Load an FiffEvoked from a FIF file.
     *
     * @param[in] evokedPath  Path to the FIF file.
     * @param[in] aveIndex    Set index to load.
     * @return The loaded FiffEvoked (may be empty on failure).
     */
    static FIFFLIB::FiffEvoked loadEvoked(const QString &evokedPath, int aveIndex = 0);

    /**
     * Probe a FIF file for the available evoked data sets.
     *
     * @param[in] evokedPath  Path to the FIF file.
     * @return List of human-readable labels for each set.
     */
    static QStringList probeEvokedSets(const QString &evokedPath);
};

#endif // DATALOADER_H
