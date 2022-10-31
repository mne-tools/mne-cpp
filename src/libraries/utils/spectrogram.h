//=============================================================================================================
/**
 * @file     spectrogram.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     September, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Lorenz Esch. All rights reserved.
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
 * @brief    Declaration of spectrogram class.
 */

#ifndef SPECTROGRAM_H
#define SPECTROGRAM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

struct SpectogramInputData {
    Eigen::VectorXd vecInputData;
    quint32 iRangeLow;
    quint32 iRangeHigh;
    qint32 window_size;
};

class UTILSSHARED_EXPORT Spectrogram
{

public:
    //=========================================================================================================
    /**
     * Calculates the spectrogram (tf-representation) of a given signal
     *
     * @param[in] signal         input-signal to calculate spectrogram of.
     * @param[in] windowSize     size of the window which is used (resolution in time an frequency is depending on it).
     *
     * @return spectrogram-matrix (tf-representation of the input signal).
     */
    static Eigen::MatrixXd makeSpectrogram(Eigen::VectorXd signal,
                                           qint32 windowSize);

private:
    //=========================================================================================================
    /**
     * Calculates a gaussean window function
     *
     * @param[in] sample_count   number of samples.
     * @param[in] scale          window width.
     * @param[in] translation    translation of the window among a signal.
     *
     * @return samples of window-vector.
     */
    static Eigen::VectorXd gaussWindow (qint32 sample_count,
                                        qreal scale,
                                        quint32 translation);

    //=========================================================================================================
    /**
     * Calculates the spectogram matrix for a given input data matrix.
     *
     * @param[in] data       The input data.
     *
     * @return               The spectogram matrix.
     */
    static Eigen::MatrixXd compute(const SpectogramInputData& data);

    //=========================================================================================================
    /**
     * Sums up (reduces) the in parallel processed spectogram matrix.
     *
     * @param[out] resultData    The result data.
     * @param[in] data          The incoming, temporary result data.
     */
    static void reduce(Eigen::MatrixXd &resultData,
                       const Eigen::MatrixXd &data);
};
}//namespace

#endif // SPECTROGRAM_H

