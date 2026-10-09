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
 * Every other translation unit in BIDSLIB derives its allowed values from
 * the constants defined here, so a future BIDS specification bump
 * (currently 1.9.x) only requires touching this header. The file groups
 * five orthogonal concerns: BIDS datatype strings (@c meg, @c eeg,
 * @c ieeg, @c anat, …), per-modality raw-data file extension
 * whitelists, bidirectional FIFF-kind ↔ BIDS channel-type maps used by
 * @ref BIDSLIB::BidsChannel and @ref BIDSLIB::BidsRawData, coordinate-system name ↔
 * @c FIFFV_COORD_* maps used by @ref BIDSLIB::BidsCoordinateSystem, and the
 * canonical entity order (@c sub, @c ses, @c task, @c acq, @c run, …)
 * consumed by @ref BIDSLIB::BIDSPath when it serialises a filename.
 *
 * Header-only and inline so the maps are constant-folded at the call
 * sites and to avoid a hidden ODR surface for translation units that
 * only need one or two entries.
 */

#ifndef BIDS_CONST_H
#define BIDS_CONST_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_constants.h>

#include <QMap>
#include <QString>
#include <QStringList>

//=============================================================================================================
// DEFINE NAMESPACE BIDSLIB
//=============================================================================================================

namespace BIDSLIB
{

//=============================================================================================================
// BIDS datatypes
//=============================================================================================================

/** BIDS datatype strings used in directory and filename construction. */
const QString BIDS_DATATYPE_MEG = QStringLiteral("meg");
const QString BIDS_DATATYPE_EEG = QStringLiteral("eeg");
const QString BIDS_DATATYPE_IEEG = QStringLiteral("ieeg");
const QString BIDS_DATATYPE_ANAT = QStringLiteral("anat");
const QString BIDS_DATATYPE_FUNC = QStringLiteral("func");
const QString BIDS_DATATYPE_DWI = QStringLiteral("dwi");
const QString BIDS_DATATYPE_PERF = QStringLiteral("perf");
const QString BIDS_DATATYPE_BEH = QStringLiteral("beh");

/**
 * All allowed BIDS electrophysiology datatypes.
 *
 * @return List containing the "meg", "eeg", and "ieeg" datatype strings.
 */
inline QStringList allowedElectrophysiologyDatatypes()
{
    return {BIDS_DATATYPE_MEG, BIDS_DATATYPE_EEG, BIDS_DATATYPE_IEEG};
}

/**
 * All allowed BIDS datatypes.
 *
 * @return List of all supported BIDS datatype strings (electrophysiology, imaging, and behavioral).
 */
inline QStringList allowedDatatypes()
{
    return {BIDS_DATATYPE_MEG, BIDS_DATATYPE_EEG, BIDS_DATATYPE_IEEG,
            BIDS_DATATYPE_ANAT, BIDS_DATATYPE_FUNC, BIDS_DATATYPE_DWI,
            BIDS_DATATYPE_PERF, BIDS_DATATYPE_BEH};
}

//=============================================================================================================
// BIDS file extensions for iEEG raw data
//=============================================================================================================

/**
 * Supported raw data file extensions for iEEG in BIDS.
 *
 * @return List of lowercase extensions including the leading dot (e.g. ".vhdr", ".edf").
 */
inline QStringList ieegAllowedExtensions()
{
    return {QStringLiteral(".vhdr"), // BrainVision header
            QStringLiteral(".edf"),  // European Data Format
            QStringLiteral(".set"),  // EEGLAB
            QStringLiteral(".mef"),  // MEF3
            QStringLiteral(".nwb")}; // NWB
}

/**
 * Supported raw data file extensions for EEG in BIDS.
 *
 * @return List of lowercase extensions including the leading dot (e.g. ".vhdr", ".bdf").
 */
inline QStringList eegAllowedExtensions()
{
    return {QStringLiteral(".vhdr"),
            QStringLiteral(".edf"),
            QStringLiteral(".bdf"),
            QStringLiteral(".set"),
            QStringLiteral(".fif")};
}

//=============================================================================================================
// Channel type mapping: MNE FIFF kind → BIDS string
//=============================================================================================================

/**
 * Maps FIFF channel kind constants to BIDS channel type strings.
 * Reference: BIDS specification Appendix VII.
 *
 * @return Map from FIFF channel kind (e.g. FIFFV_EEG_CH) to BIDS channel type (e.g. "EEG").
 */
inline QMap<int, QString> fiffKindToBidsType()
{
    return {
        {FIFFV_MEG_CH, QStringLiteral("MEGMAG")},
        {FIFFV_EEG_CH, QStringLiteral("EEG")},
        {FIFFV_STIM_CH, QStringLiteral("TRIG")},
        {FIFFV_EOG_CH, QStringLiteral("EOG")},
        {FIFFV_ECG_CH, QStringLiteral("ECG")},
        {FIFFV_EMG_CH, QStringLiteral("EMG")},
        {FIFFV_MISC_CH, QStringLiteral("MISC")},
        {FIFFV_RESP_CH, QStringLiteral("RESP")},
        {FIFFV_REF_MEG_CH, QStringLiteral("MEGREF")},
        {FIFFV_ECOG_CH, QStringLiteral("ECOG")},
        {FIFFV_SEEG_CH, QStringLiteral("SEEG")},
        {FIFFV_DBS_CH, QStringLiteral("DBS")},
    };
}

/**
 * Maps BIDS channel type strings back to FIFF channel kind constants.
 * Reference: BIDS specification Appendix VII.
 *
 * @return Map from BIDS channel type (e.g. "SEEG") to FIFF channel kind (e.g. FIFFV_SEEG_CH).
 */
inline QMap<QString, int> bidsTypeToFiffKind()
{
    return {
        {QStringLiteral("MEGMAG"), FIFFV_MEG_CH},
        {QStringLiteral("MEGGRAD"), FIFFV_MEG_CH},
        {QStringLiteral("MEGREF"), FIFFV_REF_MEG_CH},
        {QStringLiteral("EEG"), FIFFV_EEG_CH},
        {QStringLiteral("ECOG"), FIFFV_ECOG_CH},
        {QStringLiteral("SEEG"), FIFFV_SEEG_CH},
        {QStringLiteral("DBS"), FIFFV_DBS_CH},
        {QStringLiteral("EOG"), FIFFV_EOG_CH},
        {QStringLiteral("ECG"), FIFFV_ECG_CH},
        {QStringLiteral("EMG"), FIFFV_EMG_CH},
        {QStringLiteral("TRIG"), FIFFV_STIM_CH},
        {QStringLiteral("MISC"), FIFFV_MISC_CH},
        {QStringLiteral("RESP"), FIFFV_RESP_CH},
    };
}

//=============================================================================================================
// Coordinate system mappings: BIDS string ↔ FIFF coordinate frame
//=============================================================================================================

/**
 * Maps BIDS coordinate system names to FIFF coordinate frame constants.
 * Used for electrodes.tsv / coordsystem.json handling.
 *
 * @return Map from BIDS coordinate system name to FIFF coordinate frame; unknown systems map to FIFFV_COORD_UNKNOWN.
 */
inline QMap<QString, int> bidsCoordToFiffFrame()
{
    return {
        {QStringLiteral("CTF"), FIFFV_COORD_HEAD},
        {QStringLiteral("ElektaNeuromag"), FIFFV_COORD_HEAD},
        {QStringLiteral("CapTrak"), FIFFV_COORD_HEAD},
        {QStringLiteral("ACPC"), FIFFV_COORD_MRI},
        {QStringLiteral("fsaverage"), FIFFV_MNE_COORD_MNI_TAL},
        {QStringLiteral("MNI305"), FIFFV_MNE_COORD_MNI_TAL},
        {QStringLiteral("fsaverageSymm"), FIFFV_MNE_COORD_MNI_TAL},
        {QStringLiteral("Other"), FIFFV_COORD_UNKNOWN},
    };
}

/**
 * Maps FIFF coordinate frame constants to BIDS coordinate system names.
 *
 * @return Map from FIFF coordinate frame to the BIDS coordinate system name written for it.
 */
inline QMap<int, QString> fiffFrameToBidsCoord()
{
    return {
        {FIFFV_COORD_HEAD, QStringLiteral("CapTrak")},
        {FIFFV_COORD_MRI, QStringLiteral("ACPC")},
        {FIFFV_MNE_COORD_MNI_TAL, QStringLiteral("MNI305")},
        {FIFFV_COORD_UNKNOWN, QStringLiteral("Other")},
    };
}

//=============================================================================================================
// Allowed coordinate systems for iEEG
//=============================================================================================================

/**
 * Allowed coordinate systems for iEEG electrodes (BIDS-iEEG spec).
 *
 * @return List of coordinate system names accepted in an iEEG coordsystem.json.
 */
inline QStringList ieegAllowedCoordSystems()
{
    return {QStringLiteral("ACPC"),
            QStringLiteral("Talairach"),
            QStringLiteral("Other"),
            QStringLiteral("fsaverage"),
            QStringLiteral("fsaverageSymm"),
            QStringLiteral("MNI305"),
            QStringLiteral("MNI152Lin"),
            QStringLiteral("MNI152NLin2009aSym"),
            QStringLiteral("MNI152NLin2009bSym"),
            QStringLiteral("MNI152NLin2009cSym"),
            QStringLiteral("MNI152NLin2009aAsym"),
            QStringLiteral("MNI152NLin2009bAsym"),
            QStringLiteral("MNI152NLin2009cAsym"),
            QStringLiteral("individual")};
}

//=============================================================================================================
// BIDS entity ordering (for filename construction)
//=============================================================================================================

/**
 * Entity key ordering for BIDS filenames (per specification).
 *
 * @return Entity keys (e.g. "sub", "ses", "task") in the order they must appear in a filename.
 */
inline QStringList bidsEntityOrder()
{
    return {QStringLiteral("sub"),
            QStringLiteral("ses"),
            QStringLiteral("task"),
            QStringLiteral("tracksys"),
            QStringLiteral("acq"),
            QStringLiteral("run"),
            QStringLiteral("proc"),
            QStringLiteral("space"),
            QStringLiteral("recording"),
            QStringLiteral("split"),
            QStringLiteral("desc")};
}

} // namespace BIDSLIB

#endif // BIDS_CONST_H
