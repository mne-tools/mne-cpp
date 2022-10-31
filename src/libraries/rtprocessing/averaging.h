//=============================================================================================================
/**
 * @file     averaging.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.3
 * @date     June, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch. All rights reserved.
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
 * @brief     Averaging declarations.
 *
 */

#ifndef AVERAGING_RTPROCESSING_H
#define AVERAGING_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtprocessing_global.h"

#include <fiff/fiff_evoked.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffRawData;
}

//=============================================================================================================
// DEFINE NAMESPACE RTPROCESSINGLIB
//=============================================================================================================

namespace RTPROCESSINGLIB
{

//=============================================================================================================
// RTPROCESSINGLIB FORWARD DECLARATIONS
//=============================================================================================================

class FilterKernel;

//=============================================================================================================
/**
 * Computes the average for given fiff raw data.
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
 */
RTPROCESINGSHARED_EXPORT FIFFLIB::FiffEvoked computeAverage(const FIFFLIB::FiffRawData& raw,
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

//=============================================================================================================
/**
 * Computes the filtered average for given fiff raw data.
 *
 * @param[in] raw               The raw data.
 * @param[in] matEvents         The events provided in samples and event kinds.
 * @param[in] fTMinS            The start time relative to the event in seconds.
 * @param[in] fTMaxS            The end time relative to the event in seconds.
 * @param[in] eventType         The event type.
 * @param[in] bApplyBaseline    Whether to use baseline correction (mode=mean).
 * @param[in] fTBaselineFromS   The start baseline correction time relative to the event in seconds.
 * @param[in] fTBaselineToS     The end baseline correction time relative to the event in seconds.
 * @param[in] filterKernel      The filter kernel to use when reading the fiff raw data.
 * @param[in] mapReject         The thresholds per channel type to reject epochs.
 * @param[in] lExcludeChs       List of channel names to exclude.
 * @param[in] vecPicks          Which channels to pick.
 */
RTPROCESINGSHARED_EXPORT FIFFLIB::FiffEvoked computeFilteredAverage(const FIFFLIB::FiffRawData& raw,
                                                                    const Eigen::MatrixXi& matEvents,
                                                                    float fTMinS,
                                                                    float fTMaxS,
                                                                    qint32 eventType,
                                                                    bool bApplyBaseline,
                                                                    float fTBaselineFromS,
                                                                    float fTBaselineToS,
                                                                    const QMap<QString,double>& mapReject,
                                                                    const FilterKernel& filterKernel,
                                                                    const QStringList &lExcludeChs = QStringList(),
                                                                    const Eigen::RowVectorXi& vecPicks = Eigen::RowVectorXi());

} // NAMESPACE

#endif // AVERAGING_RTPROCESSING_H
