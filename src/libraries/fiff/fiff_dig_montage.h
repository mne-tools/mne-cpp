//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiff_dig_montage.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Digitizer points with the names of their EEG electrodes, read from the digitizer formats of mne.channels.
 */

#ifndef FIFFLIB_FIFF_DIG_MONTAGE_H
#define FIFFLIB_FIFF_DIG_MONTAGE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_dig_point_set.h"

#include <base/error.h>

#include <Eigen/Core>

#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Digitizer points with the names of their EEG electrodes, like mne.channels.DigMontage.
 *
 * The points come in the order of mne.channels.make_dig_montage: the cardinal points (LPA, nasion,
 * RPA), HPI coils, head shape points, then the EEG electrodes. Electrodes are numbered by the last
 * three characters of their names when all of those are numbers, and 1..n otherwise; a repeated
 * name keeps its first place and its last position. All points are in digitizer coordinates
 * (FIFFV_COORD_UNKNOWN) and in metres.
 *
 * @snippet ex_fiff_api/main.cpp fiff_dig_montage_usage
 */
struct FIFFSHARED_EXPORT FiffDigMontage
{
    FiffDigPointSet dig;               /**< The digitizer points. */
    std::vector<std::string> ch_names; /**< Names of the EEG electrodes, in the order of their points. */

    //=========================================================================================================
    /**
     * Reads a Polhemus Isotrak file, as mne.channels.read_dig_polhemus_isotrak does.
     *
     * The three fiducials become the cardinal points. The remaining points become HPI coils for an
     * .elp file and head shape points for .hsp/.eeg files, or EEG electrodes when @p ch_names names them.
     *
     * @param[in] path       The .elp, .hsp or .eeg file.
     * @param[in] ch_names   Electrode names, one per non-fiducial point, or none.
     * @param[in] unit       Unit of the file: "m", "cm" or "mm".
     *
     * @return The montage, or why the file cannot be read: Io if it cannot be opened, Format if the
     *         extension, unit or content is wrong or the number of names does not match the points.
     */
    [[nodiscard]] static mne::Expected<FiffDigMontage> read_polhemus_isotrak(const std::filesystem::path& path,
                                                                             std::span<const std::string> ch_names = {},
                                                                             std::string_view unit = "m");

    //=========================================================================================================
    /**
     * Reads the points of a Polhemus FastSCAN .txt file, as mne.channels.read_polhemus_fastscan does.
     *
     * @param[in] path             The .txt file.
     * @param[in] unit             Unit of the file: "m", "cm" or "mm".
     * @param[in] require_header   Reject files whose "%" header does not name FastSCAN.
     *
     * @return The points in metres, one per row, or why the file cannot be read: Io if it cannot be
     *         opened, Format if the extension, unit or header is wrong or a row is not three numbers.
     */
    [[nodiscard]] static mne::Expected<Eigen::MatrixX3d> read_polhemus_fastscan(const std::filesystem::path& path,
                                                                                std::string_view unit = "mm",
                                                                                bool require_header = true);

    //=========================================================================================================
    /**
     * Reads a BrainVision CapTrak .bvct file, as mne.channels.read_dig_captrak does: the electrodes
     * named Nasion, LPA and RPA are the cardinal points, every other one an EEG electrode.
     *
     * @param[in] path   The .bvct file.
     *
     * @return The montage, or why the file cannot be read: Io if it cannot be opened, Format if it
     *         cannot be parsed, an electrode has no position or a fiducial is missing.
     */
    [[nodiscard]] static mne::Expected<FiffDigMontage> read_captrak(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Reads the coordinates.xml of an EGI MFF recording, as mne.channels.read_dig_egi does.
     *
     * Sensors of type 0 become EEG electrodes named "EEG <number>", the reference (type 1) is numbered
     * after the electrodes before it, and the nasion and periauricular points (type 2) become the
     * cardinal points. Unlike mne.channels.read_dig_egi, which keeps the file's centimetres as if they
     * were metres, the positions are converted to metres (as mne.io.read_raw_egi does).
     *
     * @param[in] path   The coordinates.xml file.
     *
     * @return The montage, or why the file cannot be read: Io if it cannot be opened, Format if it
     *         cannot be parsed, a sensor has no number or position or a fiducial is missing.
     */
    [[nodiscard]] static mne::Expected<FiffDigMontage> read_egi(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Reads a Localite .csv file ("#,name,x,y,z" in millimetres after a header row), as
     * mne.channels.read_dig_localite does.
     *
     * @param[in] path     The .csv file.
     * @param[in] nasion   Name of the point that is the nasion, or empty.
     * @param[in] lpa      Name of the point that is the left preauricular point, or empty.
     * @param[in] rpa      Name of the point that is the right preauricular point, or empty.
     *
     * @return The montage, or why the file cannot be read: Io if it cannot be opened, Format if a row
     *         is malformed, NotFound if a named fiducial is missing.
     */
    [[nodiscard]] static mne::Expected<FiffDigMontage> read_localite(const std::filesystem::path& path,
                                                                     std::string_view nasion = {},
                                                                     std::string_view lpa = {},
                                                                     std::string_view rpa = {});

    //=========================================================================================================
    /**
     * Reads a Neuroscan .dat electrode file ("name number x y z" per line), as mne.channels.read_dig_dat
     * does: point numbers 78, 76 and 82 are the nasion, LPA and RPA, 67 (the centroid) is skipped, and
     * the coordinates are kept as they are.
     *
     * @param[in] path   The .dat file.
     *
     * @return The montage, or why the file cannot be read: Io if it cannot be opened, Format if a line
     *         does not hold five entries.
     */
    [[nodiscard]] static mne::Expected<FiffDigMontage> read_neuroscan_dat(const std::filesystem::path& path);
};

} // namespace mne

#endif // FIFFLIB_FIFF_DIG_MONTAGE_H
