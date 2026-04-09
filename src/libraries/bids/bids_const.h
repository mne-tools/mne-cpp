//=============================================================================================================
/**
 * @file     bids_const.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March, 2026
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
 * @brief    BIDS constants, channel type mappings, and allowed values.
 *
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
const QString BIDS_DATATYPE_MEG   = QStringLiteral("meg");
const QString BIDS_DATATYPE_EEG   = QStringLiteral("eeg");
const QString BIDS_DATATYPE_IEEG  = QStringLiteral("ieeg");
const QString BIDS_DATATYPE_ANAT  = QStringLiteral("anat");
const QString BIDS_DATATYPE_FUNC  = QStringLiteral("func");
const QString BIDS_DATATYPE_BEH   = QStringLiteral("beh");

/** All allowed BIDS electrophysiology datatypes. */
inline QStringList allowedElectrophysiologyDatatypes()
{
    return {BIDS_DATATYPE_MEG, BIDS_DATATYPE_EEG, BIDS_DATATYPE_IEEG};
}

/** All allowed BIDS datatypes. */
inline QStringList allowedDatatypes()
{
    return {BIDS_DATATYPE_MEG, BIDS_DATATYPE_EEG, BIDS_DATATYPE_IEEG,
            BIDS_DATATYPE_ANAT, BIDS_DATATYPE_FUNC, BIDS_DATATYPE_BEH};
}

//=============================================================================================================
// BIDS file extensions for iEEG raw data
//=============================================================================================================

/** Supported raw data file extensions for iEEG in BIDS. */
inline QStringList ieegAllowedExtensions()
{
    return {QStringLiteral(".vhdr"),    // BrainVision header
            QStringLiteral(".edf"),     // European Data Format
            QStringLiteral(".set"),     // EEGLAB
            QStringLiteral(".mef"),     // MEF3
            QStringLiteral(".nwb")};    // NWB
}

/** Supported raw data file extensions for EEG in BIDS. */
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
 */
inline QMap<int, QString> fiffKindToBidsType()
{
    return {
        {FIFFV_MEG_CH,      QStringLiteral("MEGMAG")},
        {FIFFV_EEG_CH,      QStringLiteral("EEG")},
        {FIFFV_STIM_CH,     QStringLiteral("TRIG")},
        {FIFFV_EOG_CH,      QStringLiteral("EOG")},
        {FIFFV_ECG_CH,      QStringLiteral("ECG")},
        {FIFFV_EMG_CH,      QStringLiteral("EMG")},
        {FIFFV_MISC_CH,     QStringLiteral("MISC")},
        {FIFFV_RESP_CH,     QStringLiteral("RESP")},
        {FIFFV_REF_MEG_CH,  QStringLiteral("MEGREF")},
        {FIFFV_ECOG_CH,     QStringLiteral("ECOG")},
        {FIFFV_SEEG_CH,     QStringLiteral("SEEG")},
        {FIFFV_DBS_CH,      QStringLiteral("DBS")},
    };
}

/**
 * Maps BIDS channel type strings back to FIFF channel kind constants.
 * Reference: BIDS specification Appendix VII.
 */
inline QMap<QString, int> bidsTypeToFiffKind()
{
    return {
        {QStringLiteral("MEGMAG"),  FIFFV_MEG_CH},
        {QStringLiteral("MEGGRAD"), FIFFV_MEG_CH},
        {QStringLiteral("MEGREF"),  FIFFV_REF_MEG_CH},
        {QStringLiteral("EEG"),     FIFFV_EEG_CH},
        {QStringLiteral("ECOG"),    FIFFV_ECOG_CH},
        {QStringLiteral("SEEG"),    FIFFV_SEEG_CH},
        {QStringLiteral("DBS"),     FIFFV_DBS_CH},
        {QStringLiteral("EOG"),     FIFFV_EOG_CH},
        {QStringLiteral("ECG"),     FIFFV_ECG_CH},
        {QStringLiteral("EMG"),     FIFFV_EMG_CH},
        {QStringLiteral("TRIG"),    FIFFV_STIM_CH},
        {QStringLiteral("MISC"),    FIFFV_MISC_CH},
        {QStringLiteral("RESP"),    FIFFV_RESP_CH},
    };
}

//=============================================================================================================
// Coordinate system mappings: BIDS string ↔ FIFF coordinate frame
//=============================================================================================================

/**
 * Maps BIDS coordinate system names to FIFF coordinate frame constants.
 * Used for electrodes.tsv / coordsystem.json handling.
 */
inline QMap<QString, int> bidsCoordToFiffFrame()
{
    return {
        {QStringLiteral("CTF"),           FIFFV_COORD_HEAD},
        {QStringLiteral("ElektaNeuromag"), FIFFV_COORD_HEAD},
        {QStringLiteral("CapTrak"),       FIFFV_COORD_HEAD},
        {QStringLiteral("ACPC"),          FIFFV_COORD_MRI},
        {QStringLiteral("fsaverage"),     FIFFV_MNE_COORD_MNI_TAL},
        {QStringLiteral("MNI305"),        FIFFV_MNE_COORD_MNI_TAL},
        {QStringLiteral("fsaverageSymm"), FIFFV_MNE_COORD_MNI_TAL},
        {QStringLiteral("Other"),         FIFFV_COORD_UNKNOWN},
    };
}

/**
 * Maps FIFF coordinate frame constants to BIDS coordinate system names.
 */
inline QMap<int, QString> fiffFrameToBidsCoord()
{
    return {
        {FIFFV_COORD_HEAD,          QStringLiteral("CapTrak")},
        {FIFFV_COORD_MRI,          QStringLiteral("ACPC")},
        {FIFFV_MNE_COORD_MNI_TAL, QStringLiteral("MNI305")},
        {FIFFV_COORD_UNKNOWN,      QStringLiteral("Other")},
    };
}

//=============================================================================================================
// Allowed coordinate systems for iEEG
//=============================================================================================================

/** Allowed coordinate systems for iEEG electrodes (BIDS-iEEG spec). */
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

/** Entity key ordering for BIDS filenames (per specification). */
inline QStringList bidsEntityOrder()
{
    return {QStringLiteral("sub"),
            QStringLiteral("ses"),
            QStringLiteral("task"),
            QStringLiteral("acq"),
            QStringLiteral("run"),
            QStringLiteral("proc"),
            QStringLiteral("space"),
            QStringLiteral("rec"),
            QStringLiteral("split"),
            QStringLiteral("desc"),
            QStringLiteral("tracking_system")};
}

} // namespace BIDSLIB

#endif // BIDS_CONST_H
