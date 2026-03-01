//=============================================================================================================
/**
 * @file     fiff_evoked_set.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    FiffEvokedSet class declaration.
 *
 */

#ifndef FIFF_EVOKED_SET_H
#define FIFF_EVOKED_SET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_info.h"
#include "fiff_evoked.h"
#include "fiff_global.h"

#include <Eigen/Core>
#include <QVector>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QIODevice>
#include <QList>
#include <QSharedPointer>
#include <QStringList>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace FIFFLIB
{

class FiffRawData;

//=============================================================================================================
/**
 * Rejection parameters for artifact detection.
 * Ported from rejDataRec (MNE-C browser_types.h).
 */
struct FIFFSHARED_EXPORT RejectionParams {
    float stimIgnore       = 0.0f;      /**< Ignore this many seconds around the stimulus. */
    float megGradReject    = 2000e-13f;  /**< Gradiometer rejection (T/m). */
    float megMagReject     = 3e-12f;     /**< Magnetometer rejection (T). */
    float eegReject        = 100e-6f;    /**< EEG rejection (V). */
    float eogReject        = 150e-6f;    /**< EOG rejection (V). */
    float ecgReject        = 0.0f;       /**< ECG rejection (V). */
    float megGradFlat      = 0.0f;      /**< Gradiometer flatness (T/m). */
    float megMagFlat       = 0.0f;      /**< Magnetometer flatness (T). */
    float eegFlat          = 0.0f;      /**< EEG flatness (V). */
    float eogFlat          = 0.0f;      /**< EOG flatness (V). */
    float ecgFlat          = 0.0f;      /**< ECG flatness (V). */
};

//=============================================================================================================
/**
 * One averaging category.
 * Ported from aveCategRec (MNE-C browser_types.h).
 */
struct FIFFSHARED_EXPORT AverageCategory {
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
struct FIFFSHARED_EXPORT AverageDescription {
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
 * Fiff evoked data set
 *
 * @brief evoked data set
 */
class FIFFSHARED_EXPORT FiffEvokedSet
{

public:
    typedef QSharedPointer<FiffEvokedSet> SPtr;             /**< Shared pointer type for FiffEvokedSet. */
    typedef QSharedPointer<const FiffEvokedSet> ConstSPtr;  /**< Const shared pointer type for FiffEvokedSet. */

    //=========================================================================================================
    /**
     * Constructs a fiff evoked data set.
     */
    FiffEvokedSet();

    //=========================================================================================================
    /**
     * Constructs a fiff evoked data set, by reading from a IO device.
     *
     * @param[in] p_IODevice     IO device to read from the evoked data set.
     */
    FiffEvokedSet(QIODevice& p_IODevice);

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffEvokedSet    Fiff evoked data set which should be copied.
     */
    FiffEvokedSet(const FiffEvokedSet& p_FiffEvokedSet);

    //=========================================================================================================
    /**
     * Destroys the fiff evoked data set.
     */
    ~FiffEvokedSet();

    //=========================================================================================================
    /**
     * Initializes fiff evoked data set.
     */
    void clear();

    //=========================================================================================================
    /**
     * fiff_pick_channels_evoked
     *
     * Pick desired channels from evoked-response data
     *
     * @param[in] include   - Channels to include (if empty, include all available).
     * @param[in] exclude   - Channels to exclude (if empty, do not exclude any).
     *
     * @return the desired fiff evoked data set.
     */
    FiffEvokedSet pick_channels(const QStringList& include = defaultQStringList,
                                const QStringList& exclude = defaultQStringList) const;

    //=========================================================================================================
    /**
     * mne_compensate_to
     *
     * Apply compensation to the data as desired
     *
     * @param[in] to                  desired compensation in the output.
     * @param[in, out] p_FiffEvokedSet Evoked set to compensate.
     *
     * @return true if succeeded, false otherwise.
     */
    bool compensate_to(FiffEvokedSet &p_FiffEvokedSet,
                       fiff_int_t to) const;

    //=========================================================================================================
    /**
     * fiff_find_evoked
     *
     * Find evoked data sets
     *
     * @param[out] p_FiffEvokedSet   The read evoked data set.
     *
     * @return true when any set was found, false otherwise.
     */
    bool find_evoked(const FiffEvokedSet& p_FiffEvokedSet) const;

    //=========================================================================================================
    /**
     * fiff_read_evoked
     *
     * Wrapper for the FiffEvokedDataSet::read_evoked static function
     *
     * Read one evoked data set
     *
     * @param[in] p_IODevice         An fiff IO device like a fiff QFile or QTCPSocket.
     * @param[out] p_FiffEvokedSet   The read evoked data set.
     * @param[in] baseline           The time interval to apply rescaling / baseline correction. If None do not apply it. If baseline is (a, b).
     *                               the interval is between "a (s)" and "b (s)". If a is None the beginning of the data is used and if b is
     *                               None then b is set to the end of the interval. If baseline is equal ot (None, None) all the time interval is used.
     *                               If None, no correction is applied.
     * @param[in] proj               Apply SSP projection vectors (optional, default = true).
     *
     * @return true when successful, false otherwise.
     */
    static bool read(QIODevice& p_IODevice,
                     FiffEvokedSet& p_FiffEvokedSet,
                     QPair<float,float> baseline = defaultFloatPair,
                     bool proj = true);

    //=========================================================================================================
    /**
     * Save this evoked data set to a FIFF file.
     *
     * @param[in] fileName  Output file path.
     * @return true on success.
     */
    bool save(const QString &fileName) const;

    //=========================================================================================================
    /**
     * Compute a grand average across multiple evoked data sets.
     * Corresponding categories are averaged by summing data and dividing by the
     * number of sets. The nave field accumulates the total count.
     *
     * @param[in] evokedSets    List of evoked data sets to combine.
     * @return The grand-average evoked set, or an empty set if input is empty.
     */
    static FiffEvokedSet computeGrandAverage(const QList<FiffEvokedSet> &evokedSets);

    //=========================================================================================================
    /**
     * Compute epoch-based averages from raw data according to an averaging description.
     *
     * For each category in the description, matching events are epoched from the raw data,
     * artifact-rejected, baseline-corrected, and averaged into a FiffEvoked entry.
     *
     * @param[in] raw           The raw data.
     * @param[in] desc          The averaging description (categories, rejection, etc.).
     * @param[in] events        Event matrix (nEvents x 3): [sample, from, to].
     * @param[out] log          Processing log output.
     * @return FiffEvokedSet containing one FiffEvoked per category, or empty set on failure.
     */
    static FiffEvokedSet computeAverages(const FiffRawData &raw,
                                         const AverageDescription &desc,
                                         const Eigen::MatrixXi &events,
                                         QString &log);

    //=========================================================================================================
    /**
     * Check whether an epoch passes artifact rejection criteria.
     *
     * @param[in] epoch     Epoch data (nChannels x nSamples).
     * @param[in] info      Channel info.
     * @param[in] bads      List of bad channel names.
     * @param[in] rej       Rejection parameters.
     * @param[out] reason   Rejection reason string (set only when rejected).
     * @return true if epoch is clean (not rejected).
     */
    static bool checkArtifacts(const Eigen::MatrixXd &epoch,
                               const FiffInfo &info,
                               const QStringList &bads,
                               const RejectionParams &rej,
                               QString &reason);

    /**
     * @brief Subtract baseline from each channel of an epoch.
     *
     * For each row (channel), the mean of the samples in [bminSamp, bminSamp+nBase)
     * is subtracted.
     *
     * @param[in,out] epoch     Data matrix (channels x samples) to correct in-place.
     * @param[in] bminSamp      First baseline sample (clamped to 0).
     * @param[in] bmaxSamp      Last baseline sample (clamped to epoch length - 1).
     */
    static void subtractBaseline(Eigen::MatrixXd &epoch, int bminSamp, int bmaxSamp);

public:
    FiffInfo             info;   /**< FIFF measurement information. */
    QList<FiffEvoked>    evoked; /**< List of Fiff Evoked Data. */
};
} // NAMESPACE

#ifndef metatype_fiffevokedset
#define metatype_fiffevokedset
Q_DECLARE_METATYPE(FIFFLIB::FiffEvokedSet);/**< Provides QT META type declaration of the FIFFLIB::FiffEvokedSet type. For signal/slot and QVariant usage.*/
#endif

#ifndef metatype_fiffevokedsetsptr
#define metatype_fiffevokedsetsptr
Q_DECLARE_METATYPE(FIFFLIB::FiffEvokedSet::SPtr);/**< Provides QT META type declaration of the FIFFLIB::FiffEvokedSet type. For signal/slot and QVariant usage.*/
#endif

#endif // FIFF_EVOKED_SET_H
