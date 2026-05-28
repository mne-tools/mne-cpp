//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     mne_epoch_data_list.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     October 2012
 * @brief    Ordered list of @ref MNELIB::MNEEpochData objects sharing a common @ref FIFFLIB::FiffInfo.
 *
 * @ref MNELIB::MNEEpochDataList is the C++ counterpart of MNE-Python's
 * @c mne.Epochs: a vector of trials cut from a continuous raw recording
 * with common pre/post-stimulus boundaries, optional artifact rejection
 * and convenience helpers to average into a @ref FIFFLIB::FiffEvoked or
 * to dump back to FIFF.
 */

#ifndef MNE_EPOCH_DATA_LIST_H
#define MNE_EPOCH_DATA_LIST_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_types.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_raw_data.h>

#include "mne_global.h"
#include "mne_epoch_data.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>
#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

/**
 * @brief Artifact rejection thresholds and flags for each channel type (grad, mag, eeg, eog) used during epoch dropping
 */
struct ArtifactRejectionData {
    bool bRejected = false;
    Eigen::RowVectorXd data;
    double dThreshold;
    QString sChName;
};

//=============================================================================================================
/**
 * Epoch data list, which corresponds to a set of events
 *
 * @brief Ordered list of @ref MNEEpochData objects sharing a common measurement info.
 */
class MNESHARED_EXPORT MNEEpochDataList : public QList<MNEEpochData::SPtr>
{

public:
    typedef QSharedPointer<MNEEpochDataList> SPtr;              /**< Shared pointer type for MNEEpochDataList. */
    typedef QSharedPointer<const MNEEpochDataList> ConstSPtr;   /**< Const shared pointer type for MNEEpochDataList. */

    //=========================================================================================================
    /**
     * Default constructor.
     */
    MNEEpochDataList();

    //=========================================================================================================
    /**
     * Destroys the MNEEpochDataList.
     */
    ~MNEEpochDataList();

    //=========================================================================================================
    /**
     * Read the epochs from a raw file based on provided events.
     *
     * @param[in] raw            The raw data.
     * @param[in] events         The events provided in samples and event kind.
     * @param[in] tmin           The start time relative to the event in seconds.
     * @param[in] tmax           The end time relative to the event in seconds.
     * @param[in] event          The event kind.
     * @param[in] lExcludeChs    List of channel names to exclude.
     * @param[in] picks          Which channels to pick.
     */
    static MNEEpochDataList readEpochs(const FIFFLIB::FiffRawData& raw,
                                       const Eigen::MatrixXi& events,
                                       float tmin,
                                       float tmax,
                                       qint32 event,
                                       const QMap<QString,double>& mapReject,
                                       const QStringList &lExcludeChs = QStringList(),
                                       const Eigen::RowVectorXi& picks = Eigen::RowVectorXi());

    //=========================================================================================================
    /**
     * Averages epoch list. Note that no baseline correction performed.
     *
     * @param[in] info     measurement info.
     * @param[in] first    First time sample.
     * @param[in] last     Last time sample.
     * @param[in] sel      Which epochs should be averaged (optional).
     * @param[in] proj     Apply SSP projection vectors (optional, default = false).
     */
    FIFFLIB::FiffEvoked average(const FIFFLIB::FiffInfo &p_info,
                                FIFFLIB::fiff_int_t first,
                                FIFFLIB::fiff_int_t last,
                                Eigen::VectorXi sel = FIFFLIB::defaultVectorXi,
                                bool proj = false) const;

    //=========================================================================================================
    /**
     * Applies baseline correction to the evoked data.
     *
     * @param[in] baseline     time definition of the baseline in seconds [from, to].
     */
    void applyBaselineCorrection(const QPair<float, float> &baseline);

    //=========================================================================================================
    /**
     * Drop/Remove all epochs tagged as rejected
     */
    void dropRejected();

    //=========================================================================================================
    /**
     * Reduces alld epochs to the selected rows.
     *
     * @param[in] sel     The selected rows to keep.
     */
    void pick_channels(const Eigen::RowVectorXi& sel);

    //=========================================================================================================
    /**
     * Checks the givven matrix for artifacts beyond a threshold value.
     *
     * @param[in] data           The data matrix.
     * @param[in] pFiffInfo      The fiff info.
     * @param[in] mapReject      The channel data types to scan for. EEG, MEG or EOG.
     * @param[in] lExcludeChs    List of channel names to exclude.
     *
     * @return   Whether a threshold artifact was detected.
     */
    static bool checkForArtifact(const Eigen::MatrixXd& data,
                                 const FIFFLIB::FiffInfo& pFiffInfo,
                                 const QMap<QString,double>& mapReject,
                                 const QStringList &lExcludeChs = QStringList());

    static void checkChThreshold(ArtifactRejectionData& inputData);

    //=========================================================================================================
    /**
     * averageCategories
     *
     * Multi-category offline averaging. Reads epochs from raw data for multiple event types
     * and returns an FiffEvokedSet with one FiffEvoked per category.
     * Ported from average.c (MNE-C).
     *
     * @param[in] raw           The raw data.
     * @param[in] events        Event matrix (nEvents x 3): [sample, before, after].
     * @param[in] eventCodes    List of event codes, one per category.
     * @param[in] comments      List of category names/comments, one per category.
     * @param[in] tmin          Start of epoch relative to event (seconds).
     * @param[in] tmax          End of epoch relative to event (seconds).
     * @param[in] mapReject     Rejection thresholds (key = channel type string, value = threshold).
     * @param[in] baseline      Baseline interval [from, to] in seconds. If from==to, no baseline correction.
     * @param[in] proj          Apply SSP projection vectors (optional, default = false).
     *
     * @return FiffEvokedSet containing one FiffEvoked per category.
     */
    static FIFFLIB::FiffEvokedSet averageCategories(const FIFFLIB::FiffRawData &raw,
                                                     const Eigen::MatrixXi &events,
                                                     const QList<int> &eventCodes,
                                                     const QStringList &comments,
                                                     float tmin,
                                                     float tmax,
                                                     const QMap<QString,double> &mapReject = QMap<QString,double>(),
                                                     const QPair<float,float> &baseline = QPair<float,float>(0.0f, 0.0f),
                                                     bool proj = false);

    //=========================================================================================================
    /**
     * Convenience function: reads epochs, optionally applies baseline correction and
     * artifact rejection, then returns the averaged evoked response.
     *
     * @param[in] raw               The raw data.
     * @param[in] matEvents         The events provided in samples and event kinds.
     * @param[in] fTMinS            The start time relative to the event in seconds.
     * @param[in] fTMaxS            The end time relative to the event in seconds.
     * @param[in] eventType         The event type.
     * @param[in] bApplyBaseline    Whether to use baseline correction (mode=mean).
     * @param[in] fTBaselineFromS   The start baseline correction time relative to the event in seconds.
     * @param[in] fTBaselineToS     The end baseline correction time relative to the event in seconds.
     * @param[in] mapReject         The thresholds per channel type to reject epochs.
     * @param[in] lExcludeChs       List of channel names to exclude.
     * @param[in] vecPicks          Which channels to pick.
     *
     * @return The averaged evoked data.
     */
    static FIFFLIB::FiffEvoked computeAverage(const FIFFLIB::FiffRawData& raw,
                                              const Eigen::MatrixXi& matEvents,
                                              float fTMinS,
                                              float fTMaxS,
                                              qint32 eventType,
                                              bool bApplyBaseline,
                                              float fTBaselineFromS,
                                              float fTBaselineToS,
                                              const QMap<QString,double>& mapReject,
                                              const QStringList &lExcludeChs = QStringList(),
                                              const Eigen::RowVectorXi& vecPicks = Eigen::RowVectorXi());
};
} // NAMESPACE

#endif // MNE_EPOCH_DATA_LIST_H
