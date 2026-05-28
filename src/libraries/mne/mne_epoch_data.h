//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Christof Pieloth <pieloth@labp.htwk-leipzig.de>
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file mne_epoch_data.h
 * @since October 2012
 * @brief Single epoch (one trial slice of preprocessed sensor data) with timing and rejection metadata.
 *
 * @ref MNELIB::MNEEpochData stores the windowed sensor matrix for one
 * event together with the trigger sample, event id, baseline boundaries
 * and any rejection thresholds applied to produce it. Multiple epochs
 * are aggregated into @ref MNEEpochDataList which then feeds covariance
 * estimation, average-by-condition evoked computation and induced-power
 * analyses.
 */

#ifndef MNE_EPOCH_DATA_H
#define MNE_EPOCH_DATA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

#include <fiff/fiff_types.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
/**
 * Epoch data, which corresponds to an event
 *
 * @brief Single epoch (trial slice) of sensor data with timing and rejection metadata.
 */
class MNESHARED_EXPORT MNEEpochData
{

public:
    typedef QSharedPointer<MNEEpochData> SPtr;              /**< Shared pointer type for MNEEpochData. */
    typedef QSharedPointer<const MNEEpochData> ConstSPtr;   /**< Const shared pointer type for MNEEpochData. */

    //=========================================================================================================
    /**
     * Default constructor.
     */
    MNEEpochData();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_MNEEpochData     MNE epoch data.
     */
    MNEEpochData(const MNEEpochData &p_MNEEpochData);

    //=========================================================================================================
    /**
     * Destroys the MNE epoch data.
     */
    ~MNEEpochData();

    //=========================================================================================================
    /**
     * Applies baseline (mode=mean) correction to the evoked data.
     *
     * @param[in] baseline     time definition of the baseline in seconds [from, to].
     */
    void applyBaselineCorrection(const QPair<float,float>& baseline);

        //=========================================================================================================
    /**
     * Reduces the data to the selected rows.
     *
     * @param[in] sel     The selected rows to keep.
     */
    void pick_channels(const Eigen::RowVectorXi& sel);

    //=========================================================================================================
    /**
     * Comparison of two Epoch data
     *
     * @param[in] b     Epoch data to compare with.
     *
     * @return true if equal; false otherwise.
     */
    friend bool operator== (const MNEEpochData &a, const MNEEpochData &b)
    {
        return (a.epoch == b.epoch &&
                a.event == b.event&&
                a.eventSample == b.eventSample &&
                a.tmin == b.tmin&&
                a.tmax == b.tmax&&
                a.bReject == b.bReject &&
                a.bUserReject == b.bUserReject);
    }

    //=========================================================================================================
    /**
     * Returns whether this epoch should be excluded from averaging.
     *
     * @param[in] respectAutoReject  Whether automatic artifact rejection should be honored.
     *
     * @return true when the epoch should be excluded.
     */
    inline bool isRejected(bool respectAutoReject = true) const
    {
        return bUserReject || (respectAutoReject && bReject);
    }

public:
    Eigen::MatrixXd     epoch;          /**< The data. */
    FIFFLIB::fiff_int_t event;          /**< The event code. */
    FIFFLIB::fiff_int_t eventSample;    /**< The sample index of the triggering event. */
    float               tmin;           /**< New start time (must be >= 0). */
    float               tmax;           /**< New end time of the data (cannot exceed data duration). */
    bool                bReject;        /**< Whether this epoch is to be rejected. */
    bool                bUserReject;    /**< Whether this epoch was manually excluded by the user. */
};
} // NAMESPACE

#endif // MNE_EPOCH_DATA_H
