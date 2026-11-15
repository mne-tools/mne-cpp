//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_electrode.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Reader/writer for the BIDS ``_electrodes.tsv`` sidecar plus a bridge into FIFF digitizer-point space.
 *
 * @c _electrodes.tsv lists one row per implanted/positioned electrode
 * with REQUIRED @c name and @c x / @c y / @c z coordinates plus
 * optional @c size / @c type / @c material / @c impedance metadata.
 * Coordinates are interpreted in the frame named by the sibling
 * @c _coordsystem.json (see @ref mne::BidsCoordinateSystem) using the unit
 * declared there.
 *
 * @ref mne::BidsElectrode::to_fiff_dig_points "BidsElectrode::to_fiff_dig_points" converts a list of records into a
 * @c mne::FiffDigPointSet of EEG digitizer points so the rest of
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
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <span>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Electrode position record corresponding to one row in *_electrodes.tsv.
 *
 * @snippet ex_bids/main.cpp bids_sidecar_records
 */
struct BIDSSHARED_EXPORT BidsElectrode
{
    std::string name;      /**< Electrode name (REQUIRED). */
    std::string x;         /**< X coordinate or "n/a" (REQUIRED). */
    std::string y;         /**< Y coordinate or "n/a" (REQUIRED). */
    std::string z;         /**< Z coordinate or "n/a" (REQUIRED). */
    std::string size;      /**< Electrode size in mm or "n/a" (RECOMMENDED for iEEG). */
    std::string type;      /**< Electrode type: "depth", "strip", "grid" (OPTIONAL). */
    std::string material;  /**< Electrode material (OPTIONAL). */
    std::string impedance; /**< Impedance value or "n/a" (OPTIONAL). */
    std::string group;     /**< iEEG: lead (shaft, strip or grid) the contact belongs to (OPTIONAL). */
    std::string dimension; /**< iEEG: shape of that group, e.g. "[1x8]" (OPTIONAL). */

    /**
     * @brief Read a BIDS *_electrodes.tsv file.
     * @param[in] path       Path to the electrodes.tsv file.
     * @return List of electrode records.
     */
    static std::vector<BidsElectrode> read_tsv(const std::filesystem::path& path);

    /**
     * @brief Write a BIDS *_electrodes.tsv file.
     * @param[in] path         Output path.
     * @param[in] electrodes   List of electrode records.
     * @return true on success.
     */
    static bool write_tsv(const std::filesystem::path& path,
                          std::span<const BidsElectrode> electrodes);

    /**
     * @brief Convert a list of BIDS electrodes to a FIFF digitizer point set.
     *
     * Each electrode with valid numeric x/y/z coordinates is converted to a
     * FiffDigPoint with kind FIFFV_POINT_EEG. Electrodes with "n/a" coordinates
     * are skipped.
     *
     * If a coordinate transform is supplied, the positions are transformed from
     * the BIDS coordinate frame into the FIFF head frame before insertion, and the
     * points are in its target frame; otherwise their frame is FIFFV_COORD_UNKNOWN.
     *
     * @param[in] electrodes    List of BIDS electrode records.
     * @param[in] trans         Optional coordinate transform (BIDS frame → head).
     *                          Pass a default-constructed FiffCoordTrans to skip.
     * @return FiffDigPointSet with one point per valid electrode.
     */
    static mne::FiffDigPointSet to_fiff_dig_points(
        std::span<const BidsElectrode> electrodes,
        const mne::FiffCoordTrans& trans = mne::FiffCoordTrans());
};

} // namespace mne

#endif // BIDS_ELECTRODE_H
