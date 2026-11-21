//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_const.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Centralised BIDS vocabulary: datatype / suffix / extension whitelists, FIFF↔BIDS channel-type and coordinate-frame maps, and entity ordering.
 *
 * Every other translation unit in mne_bids derives its allowed values from
 * the constants defined here, so a future BIDS specification bump
 * (currently 1.9.x) only requires touching this header. The file groups
 * five orthogonal concerns: BIDS datatype strings (@c meg, @c eeg,
 * @c ieeg, @c anat, …), per-modality raw-data file extension
 * whitelists, bidirectional FIFF-kind ↔ BIDS channel-type maps used by
 * @ref mne::BidsChannel and @ref mne::BidsRawData, coordinate-system name ↔
 * @c FIFFV_COORD_* maps used by @ref mne::BidsCoordinateSystem, and the
 * canonical entity order (@c sub, @c ses, @c task, @c acq, @c run, …)
 * consumed by @ref mne::BIDSPath when it serialises a filename.
 *
 * Header-only and constexpr, so the tables live in read-only data and
 * lookups need no allocation.
 */

#ifndef BIDS_CONST_H
#define BIDS_CONST_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_constants.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <optional>
#include <string_view>
#include <utility>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
// BIDS datatypes
//=============================================================================================================

/** BIDS datatype strings used in directory and filename construction. */
inline constexpr std::string_view BIDS_DATATYPE_MEG = "meg";
inline constexpr std::string_view BIDS_DATATYPE_EEG = "eeg";
inline constexpr std::string_view BIDS_DATATYPE_IEEG = "ieeg";
inline constexpr std::string_view BIDS_DATATYPE_ANAT = "anat";
inline constexpr std::string_view BIDS_DATATYPE_FUNC = "func";
inline constexpr std::string_view BIDS_DATATYPE_DWI = "dwi";
inline constexpr std::string_view BIDS_DATATYPE_PERF = "perf";
inline constexpr std::string_view BIDS_DATATYPE_BEH = "beh";

/** All allowed BIDS electrophysiology datatypes: "meg", "eeg" and "ieeg". */
inline constexpr std::array<std::string_view, 3> BIDS_ELECTROPHYSIOLOGY_DATATYPES{BIDS_DATATYPE_MEG, BIDS_DATATYPE_EEG, BIDS_DATATYPE_IEEG};

/** All allowed BIDS datatypes (electrophysiology, imaging and behavioral). */
inline constexpr std::array<std::string_view, 8> BIDS_DATATYPES{BIDS_DATATYPE_MEG, BIDS_DATATYPE_EEG, BIDS_DATATYPE_IEEG,
                                                                BIDS_DATATYPE_ANAT, BIDS_DATATYPE_FUNC, BIDS_DATATYPE_DWI,
                                                                BIDS_DATATYPE_PERF, BIDS_DATATYPE_BEH};

/** Supported raw data file extensions for iEEG in BIDS, lowercase with the leading dot. */
inline constexpr std::array<std::string_view, 5> BIDS_IEEG_EXTENSIONS{".vhdr", // BrainVision header
                                                                      ".edf",  // European Data Format
                                                                      ".set",  // EEGLAB
                                                                      ".mef",  // MEF3
                                                                      ".nwb"}; // NWB

/** Supported raw data file extensions for EEG in BIDS, lowercase with the leading dot. */
inline constexpr std::array<std::string_view, 5> BIDS_EEG_EXTENSIONS{".vhdr", ".edf", ".bdf", ".set", ".fif"};

//=============================================================================================================
// Channel type mapping (FIFF kind <-> BIDS type), BIDS specification Appendix VII
//=============================================================================================================

/** BIDS channel type of each FIFF channel kind written to channels.tsv. */
inline constexpr std::array<std::pair<int, std::string_view>, 12> FIFF_KIND_TO_BIDS_TYPE{{
    {FIFFV_MEG_CH, "MEGMAG"},
    {FIFFV_EEG_CH, "EEG"},
    {FIFFV_STIM_CH, "TRIG"},
    {FIFFV_EOG_CH, "EOG"},
    {FIFFV_ECG_CH, "ECG"},
    {FIFFV_EMG_CH, "EMG"},
    {FIFFV_MISC_CH, "MISC"},
    {FIFFV_RESP_CH, "RESP"},
    {FIFFV_REF_MEG_CH, "MEGREF"},
    {FIFFV_ECOG_CH, "ECOG"},
    {FIFFV_SEEG_CH, "SEEG"},
    {FIFFV_DBS_CH, "DBS"},
}};

/** FIFF channel kind of each BIDS channel type read from channels.tsv. */
inline constexpr std::array<std::pair<std::string_view, int>, 13> BIDS_TYPE_TO_FIFF_KIND{{
    {"MEGMAG", FIFFV_MEG_CH},
    {"MEGGRAD", FIFFV_MEG_CH},
    {"MEGREF", FIFFV_REF_MEG_CH},
    {"EEG", FIFFV_EEG_CH},
    {"ECOG", FIFFV_ECOG_CH},
    {"SEEG", FIFFV_SEEG_CH},
    {"DBS", FIFFV_DBS_CH},
    {"EOG", FIFFV_EOG_CH},
    {"ECG", FIFFV_ECG_CH},
    {"EMG", FIFFV_EMG_CH},
    {"TRIG", FIFFV_STIM_CH},
    {"MISC", FIFFV_MISC_CH},
    {"RESP", FIFFV_RESP_CH},
}};

//=============================================================================================================
// Coordinate system mapping
//=============================================================================================================

/** FIFF coordinate frame of each BIDS coordinate system name read from electrodes.tsv / coordsystem.json. */
inline constexpr std::array<std::pair<std::string_view, int>, 8> BIDS_COORD_TO_FIFF_FRAME{{
    {"CTF", FIFFV_COORD_HEAD},
    {"ElektaNeuromag", FIFFV_COORD_HEAD},
    {"CapTrak", FIFFV_COORD_HEAD},
    {"ACPC", FIFFV_COORD_MRI},
    {"fsaverage", FIFFV_MNE_COORD_MNI_TAL},
    {"MNI305", FIFFV_MNE_COORD_MNI_TAL},
    {"fsaverageSymm", FIFFV_MNE_COORD_MNI_TAL},
    {"Other", FIFFV_COORD_UNKNOWN},
}};

/** BIDS coordinate system name written for each FIFF coordinate frame. */
inline constexpr std::array<std::pair<int, std::string_view>, 4> FIFF_FRAME_TO_BIDS_COORD{{
    {FIFFV_COORD_HEAD, "CapTrak"},
    {FIFFV_COORD_MRI, "ACPC"},
    {FIFFV_MNE_COORD_MNI_TAL, "MNI305"},
    {FIFFV_COORD_UNKNOWN, "Other"},
}};

/** Allowed coordinate systems for iEEG electrodes (BIDS-iEEG spec). */
inline constexpr std::array<std::string_view, 14> BIDS_IEEG_COORD_SYSTEMS{"ACPC",
                                                                          "Talairach",
                                                                          "Other",
                                                                          "fsaverage",
                                                                          "fsaverageSymm",
                                                                          "MNI305",
                                                                          "MNI152Lin",
                                                                          "MNI152NLin2009aSym",
                                                                          "MNI152NLin2009bSym",
                                                                          "MNI152NLin2009cSym",
                                                                          "MNI152NLin2009aAsym",
                                                                          "MNI152NLin2009bAsym",
                                                                          "MNI152NLin2009cAsym",
                                                                          "individual"};

/** Entity keys in the order they must appear in a BIDS filename (per specification). */
inline constexpr std::array<std::string_view, 11> BIDS_ENTITY_ORDER{"sub", "ses", "task", "tracksys", "acq", "run",
                                                                    "proc", "space", "recording", "split", "desc"};

//=============================================================================================================
/**
 * Looks a key up in one of the mapping tables above.
 *
 * @param[in] table   A table of key-value pairs.
 * @param[in] key     The key.
 *
 * @return The value of @p key, or nothing if the table does not have it.
 */
template<typename Table, typename Key>
constexpr auto bids_lookup(const Table& table, const Key& key) -> std::optional<typename Table::value_type::second_type>
{
    const auto it = std::ranges::find(table, key, &Table::value_type::first);
    return it == table.end() ? std::nullopt : std::optional(it->second);
}

} // namespace mne

#endif // BIDS_CONST_H
