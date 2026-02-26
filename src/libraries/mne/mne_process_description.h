//=============================================================================================================
/**
 * @file     mne_process_description.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    2.0.0
 * @date     February, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Data types for MNE-C style averaging and covariance description files.
 *           Ported from MNE-C browser_types.h by Matti Hamalainen.
 *
 */

#ifndef MNE_PROCESS_DESCRIPTION_H
#define MNE_PROCESS_DESCRIPTION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

#include <QString>
#include <QStringList>
#include <QVector>
#include <QList>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
/**
 * Rejection parameters for artifact detection.
 * Ported from rejDataRec (MNE-C browser_types.h).
 */
struct MNESHARED_EXPORT RejectionParams {
    float stimIgnore       = 0.0f;      /**< Ignore this many seconds around the stimulus. */
    float megGradReject    = 2000e-13f;  /**< Gradiometer rejection (T/m). */
    float megMagReject     = 3e-12f;     /**< Magnetometer rejection (T). */
    float eegReject        = 100e-6f;    /**< EEG rejection (V). */
    float eogReject        = 150e-6f;    /**< EOG rejection (V). */
    float ecgReject        = 0.0f;       /**< ECG rejection (V). */
    float megGradFlat      = 0.0f;       /**< Gradiometer flatness (T/m). */
    float megMagFlat       = 0.0f;       /**< Magnetometer flatness (T). */
    float eegFlat          = 0.0f;       /**< EEG flatness (V). */
    float eogFlat          = 0.0f;       /**< EOG flatness (V). */
    float ecgFlat          = 0.0f;       /**< ECG flatness (V). */
};

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
 * One averaging category.
 * Ported from aveCategRec (MNE-C browser_types.h).
 */
struct MNESHARED_EXPORT AverageCategory {
    QString comment;                     /**< Description. */
    QVector<unsigned int> events;        /**< The interesting events. */
    unsigned int nextEvent  = 0;         /**< Require this event next. */
    unsigned int prevEvent  = 0;         /**< Require this event just before. */
    unsigned int ignore     = 0;         /**< Which trigger lines to ignore. */
    unsigned int prevIgnore = 0;         /**< Ignore mask for previous event. */
    unsigned int nextIgnore = 0;         /**< Ignore mask for next event. */
    float delay             = 0.0f;      /**< Stimulus delay (s). */
    float tmin              = -0.2f;     /**< Minimum time (s). */
    float tmax              = 0.5f;      /**< Maximum time (s). */
    float bmin              = 0.0f;      /**< Baseline min (s). */
    float bmax              = 0.0f;      /**< Baseline max (s). */
    bool  doBaseline        = false;     /**< Should we baseline? */
    bool  doStdErr          = false;     /**< Compute std error of mean? */
    bool  doAbs             = false;     /**< Compute absolute values? */
    float color[3]          = {0,0,0};   /**< Display color (unused in batch). */
};

//=============================================================================================================
/**
 * Set of averaging categories loaded from a description file.
 * Ported from aveDataRec (MNE-C browser_types.h).
 */
struct MNESHARED_EXPORT AverageDescription {
    QString comment;                     /**< Description. */
    QList<AverageCategory> categories;   /**< The categories. */
    RejectionParams rej;                 /**< Rejection limits. */
    bool fixSkew            = false;     /**< Fix skew on trigger lines. */
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
