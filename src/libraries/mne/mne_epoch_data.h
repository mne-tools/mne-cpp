//=============================================================================================================
/**
 * @file     mne_epoch_data.h
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
 * @brief     MNEEpochData class declaration.
 *
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
 * @brief epoch data
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
                a.tmin == b.tmin&&
                a.tmax == b.tmax&&
                a.bReject == b.bReject);
    }

public:
    Eigen::MatrixXd     epoch;          /**< The data. */
    FIFFLIB::fiff_int_t event;          /**< The event code. */
    float               tmin;           /**< New start time (must be >= 0). */
    float               tmax;           /**< New end time of the data (cannot exceed data duration). */
    bool                bReject;        /**< Whether this epoch is to be rejected. */
};
} // NAMESPACE

#endif // MNE_EPOCH_DATA_H
