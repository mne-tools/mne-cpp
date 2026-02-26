//=============================================================================================================
/**
 * @file     mne_averaging.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     February, 2026
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
 * @brief    Offline averaging pipeline.
 *           Ported from average.c by Matti Hamalainen.
 *
 */

#ifndef MNE_AVERAGING_H
#define MNE_AVERAGING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_process_description.h"

#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_evoked_set.h>

#include <Eigen/Core>

#include <QString>
#include <QList>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
/**
 * Provides offline averaging functionality driven by description files.
 * Ported from average.c (MNE-C).
 */
class MNESHARED_EXPORT MNEAveraging
{
public:
    /**
     * Compute averages for all categories defined in an AverageDescription.
     *
     * @param[in] raw           The raw data.
     * @param[in] desc          The averaging description (categories, rejection, etc.).
     * @param[in] events        Event matrix (nEvents x 3): [sample, before, after].
     * @param[out] log          Processing log output.
     * @return FiffEvokedSet containing one FiffEvoked per category, or empty set on failure.
     */
    static FIFFLIB::FiffEvokedSet computeAverages(const FIFFLIB::FiffRawData &raw,
                                                   const AverageDescription &desc,
                                                   const Eigen::MatrixXi &events,
                                                   QString &log);

private:
    /**
     * Check whether an epoch passes artifact rejection criteria.
     *
     * @param[in] epoch     Epoch data (nChannels x nSamples).
     * @param[in] info      Channel info.
     * @param[in] bads      List of bad channel names.
     * @param[in] rej       Rejection parameters.
     * @param[out] reason   Rejection reason string.
     * @return true if epoch is clean (not rejected).
     */
    static bool checkArtifacts(const Eigen::MatrixXd &epoch,
                               const FIFFLIB::FiffInfo &info,
                               const QStringList &bads,
                               const RejectionParams &rej,
                               QString &reason);

    /**
     * Check whether an event matches a category definition.
     *
     * @param[in] cat       The category definition.
     * @param[in] events    Full event matrix.
     * @param[in] eventIdx  Index of the current event.
     * @return true if the event matches.
     */
    static bool matchEvent(const AverageCategory &cat,
                           const Eigen::MatrixXi &events,
                           int eventIdx);

    /**
     * Subtract baseline from an epoch.
     *
     * @param[in,out] epoch     Epoch data (nChannels x nSamples).
     * @param[in] bminSamp      Baseline start (sample index relative to epoch start).
     * @param[in] bmaxSamp      Baseline end (sample index relative to epoch start).
     */
    static void subtractBaseline(Eigen::MatrixXd &epoch, int bminSamp, int bmaxSamp);
};

} // namespace MNELIB

#endif // MNE_AVERAGING_H
