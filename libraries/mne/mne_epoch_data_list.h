//=============================================================================================================
/**
 * @file     mne_epoch_data_list.h
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
 * @brief     MNEEpochDataList class declaration.
 *
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
 * @brief Epoch data list
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
                                bool proj = false);

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
};
} // NAMESPACE

#endif // MNE_EPOCH_DATA_LIST_H
