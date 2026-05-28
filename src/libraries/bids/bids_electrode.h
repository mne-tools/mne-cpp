//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file bids_electrode.h
 * @since March 2026
 * @brief Reader/writer for the BIDS ``_electrodes.tsv`` sidecar plus a bridge into FIFF digitizer-point space.
 *
 * @c _electrodes.tsv lists one row per implanted/positioned electrode
 * with REQUIRED @c name and @c x / @c y / @c z coordinates plus
 * optional @c size / @c type / @c material / @c impedance metadata.
 * Coordinates are interpreted in the frame named by the sibling
 * @c _coordsystem.json (see @ref BidsCoordinateSystem) using the unit
 * declared there.
 *
 * @ref BidsElectrode::toFiffDigPoints converts a list of records into a
 * @c FIFFLIB::FiffDigPointSet of EEG digitizer points so the rest of
 * MNE-CPP — channel registration, forward modelling, 3-D rendering —
 * can consume BIDS electrodes through the same interface it uses for
 * FIFF digitization. An optional @c FiffCoordTrans is applied while
 * converting so callers can request the points directly in head
 * coordinates. Rows whose coordinate columns are @c n/a are silently
 * dropped.
 *
 * Spec: https://bids-specification.readthedocs.io/en/stable/modality-specific-files/electrophysiology.html#electrodes-description
 */

#ifndef BIDS_ELECTRODE_H
#define BIDS_ELECTRODE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_global.h"

//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff_dig_point.h>
#include <fiff/fiff_dig_point_set.h>
#include <fiff/fiff_coord_trans.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QList>

//=============================================================================================================
// DEFINE NAMESPACE BIDSLIB
//=============================================================================================================

namespace BIDSLIB
{

//=============================================================================================================
/**
 * @brief Electrode position record corresponding to one row in *_electrodes.tsv.
 */
struct BIDSSHARED_EXPORT BidsElectrode
{
    QString name;       /**< Electrode name (REQUIRED). */
    QString x;          /**< X coordinate or "n/a" (REQUIRED). */
    QString y;          /**< Y coordinate or "n/a" (REQUIRED). */
    QString z;          /**< Z coordinate or "n/a" (REQUIRED). */
    QString size;       /**< Electrode size in mm or "n/a" (RECOMMENDED for iEEG). */
    QString type;       /**< Electrode type: "depth", "strip", "grid" (OPTIONAL). */
    QString material;   /**< Electrode material (OPTIONAL). */
    QString impedance;  /**< Impedance value or "n/a" (OPTIONAL). */

    /**
     * @brief Read a BIDS *_electrodes.tsv file.
     * @param[in] sFilePath  Path to the electrodes.tsv file.
     * @return List of electrode records.
     */
    static QList<BidsElectrode> readTsv(const QString& sFilePath);

    /**
     * @brief Write a BIDS *_electrodes.tsv file.
     * @param[in] sFilePath    Output path.
     * @param[in] electrodes   List of electrode records.
     * @return true on success.
     */
    static bool writeTsv(const QString& sFilePath,
                         const QList<BidsElectrode>& electrodes);

    /**
     * @brief Convert a list of BIDS electrodes to a FIFF digitizer point set.
     *
     * Each electrode with valid numeric x/y/z coordinates is converted to a
     * FiffDigPoint with kind FIFFV_POINT_EEG. Electrodes with "n/a" coordinates
     * are skipped.
     *
     * If a coordinate transform is supplied, the positions are transformed from
     * the BIDS coordinate frame into the FIFF head frame before insertion.
     *
     * @param[in] electrodes    List of BIDS electrode records.
     * @param[in] trans         Optional coordinate transform (BIDS frame → head).
     *                          Pass a default-constructed FiffCoordTrans to skip.
     * @return FiffDigPointSet with one point per valid electrode.
     */
    static FIFFLIB::FiffDigPointSet toFiffDigPoints(
        const QList<BidsElectrode>& electrodes,
        const FIFFLIB::FiffCoordTrans& trans = FIFFLIB::FiffCoordTrans());
};

} // namespace BIDSLIB

#endif // BIDS_ELECTRODE_H
