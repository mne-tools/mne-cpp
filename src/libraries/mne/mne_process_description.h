//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_process_description.h
 * @since 2026
 * @date  March 2026
 * @brief Plain-old-data record describing one MNE-C @c .ave / @c .cov processing entry.
 *
 * Mirrors the description entries used by @c mne_process_raw to declare
 * epoching and covariance computations (event id, baseline, time range,
 * rejection thresholds). Reproduced verbatim so the existing description
 * files continue to drive mne-cpp's batch processing without
 * reformatting.
 */

#ifndef MNE_PROCESS_DESCRIPTION_H
#define MNE_PROCESS_DESCRIPTION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

#include <fiff/fiff_evoked_set.h>

#include <QString>
#include <QStringList>
#include <QVector>
#include <QList>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

// Import averaging types from FIFFLIB for backward compatibility
using FIFFLIB::RejectionParams;
using FIFFLIB::AverageCategory;
using FIFFLIB::AverageDescription;

//=============================================================================================================
/**
 * Covariance matrix computation definition.
 * Ported from covDefRec (MNE-C browser_types.h).
 */
struct MNESHARED_EXPORT CovDefinition {
    QVector<unsigned int> events;       /**< The interesting events. */
    unsigned int ignore    = 0;         /**< Which trigger lines to ignore. */
    float delay            = 0.0f;      /**< Apply this delay to the stimulus time point. */
    float tmin             = -0.2f;     /**< Minimum time (s). */
    float tmax             = 0.0f;      /**< Maximum time (s). */
    float bmin             = 0.0f;      /**< Baseline min (s). */
    float bmax             = 0.0f;      /**< Baseline max (s). */
    bool  doBaseline       = false;     /**< Should we baseline first? */
};

//=============================================================================================================
/**
 * Covariance matrix computation specification.
 * Ported from covDataRec (MNE-C browser_types.h).
 */
struct MNESHARED_EXPORT CovDescription {
    QList<CovDefinition> defs;           /**< Definition sections. */
    RejectionParams      rej;            /**< Rejection limits. */
    bool  removeSampleMean = true;       /**< Remove the mean at each sample. */
    bool  fixSkew          = false;      /**< Fix skew on trigger lines. */
    QString filename;                    /**< Output file. */
    QString eventFile;                   /**< Read events from here. */
    QString logFile;                     /**< Save log here. */
};

//=============================================================================================================
/**
 * Filter settings.
 * Ported from mneFilterDefRec (MNE-C mne_types.h).
 */
struct MNESHARED_EXPORT FilterSettings {
    bool  filterOn          = true;      /**< Filter active? */
    int   filterSize        = 4096;      /**< FFT size. */
    int   taperSize         = 2048;      /**< Taper size. */
    float highpass          = 0.0f;      /**< Highpass corner (Hz). */
    float highpassWidth     = 0.0f;      /**< Highpass transition width (Hz). */
    float lowpass           = 40.0f;     /**< Lowpass corner (Hz). */
    float lowpassWidth      = 5.0f;      /**< Lowpass transition width (Hz). */
    float eogHighpass       = 0.0f;      /**< EOG highpass corner (Hz). */
    float eogHighpassWidth  = 0.0f;      /**< EOG highpass transition width (Hz). */
    float eogLowpass        = 40.0f;     /**< EOG lowpass corner (Hz). */
    float eogLowpassWidth   = 5.0f;      /**< EOG lowpass transition width (Hz). */
};

//=============================================================================================================
/**
 * Complete batch processing settings.
 * Combines all parameters for an MNE-C style batch processing pipeline.
 */
struct MNESHARED_EXPORT ProcessingSettings {
    // Working directory
    QString workingDir;

    // Input
    QStringList rawFiles;               /**< Raw data file(s). */
    QStringList eventFiles;             /**< Event file(s) to read. */
    int  compensateTo      = -1;        /**< Desired software gradient compensation (-1 = from file). */
    bool allowMaxShield    = false;     /**< Allow unprocessed MaxShield data. */

    // Filter
    FilterSettings filter;

    // Events output
    QStringList eventsOutFiles;         /**< Save events to these files. */
    bool saveAllEvents     = false;     /**< Save all transitions (not just leading edge). */
    QString digTrigger;                 /**< Digital trigger channel name. */
    unsigned int digTriggerMask = 0;    /**< Mask for digital trigger channel. */

    // Projection
    QStringList projFiles;              /**< SSP data files. */
    int  projOn            = -1;        /**< -1=unspecified, 0=off, 1=on. */
    bool makeProj          = false;     /**< Create a new projection operator. */
    int  projEvent         = -1;        /**< Which event for projector creation. */
    float projTmin         = -1.0f;     /**< Start time for projection calculation. */
    float projTmax         = -1.0f;     /**< End time for projection calculation. */
    int  projNGrad         = 5;         /**< Number of grad components. */
    int  projNMag          = 8;         /**< Number of mag components. */
    int  projNEeg          = 0;         /**< Number of EEG components. */
    float projGradReject   = 2000e-13f; /**< Gradiometer rejection for projection. */
    float projMagReject    = 3e-12f;    /**< Magnetometer rejection for projection. */
    float projEegReject    = 50e-6f;    /**< EEG rejection for projection. */
    QString saveProjTag;                /**< Tag for projection output files. */
    bool saveProjAugmented = false;     /**< Output in augmented format for graph. */

    // Save
    QStringList saveFiles;              /**< Destination(s) for saving filtered raw data. */
    bool omitSubjectInfo   = false;     /**< Omit subject info from output. */
    int  decimation        = 1;         /**< Decimation factor. */
    long splitSize         = -1;        /**< Split output into chunks of this many bytes (-1=no split). */

    // Averaging
    QStringList aveFiles;               /**< Averaging description file(s). */
    QString saveAveTag;                 /**< Tag for average output files. */
    QString grandAveFile;               /**< Grand average output file. */

    // Covariance
    QStringList covFiles;               /**< Covariance description file(s). */
    QString saveCovTag;                 /**< Tag for covariance output files. */
    QString grandCovFile;               /**< Grand covariance output file. */

    // Misc
    bool saveHere          = false;     /**< Save auto-generated files in CWD instead of raw data dir. */
};

} // namespace MNELIB

#endif // MNE_PROCESS_DESCRIPTION_H
