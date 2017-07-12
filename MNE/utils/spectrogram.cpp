//=============================================================================================================
/**
* @file     spectrogram.cpp
* @author   Martin Henfling <martin.henfling@tu-ilmenau.de>;
*           Daniel Knobl <daniel.knobl@tu-ilmenau.de>;
* @version  1.0
* @date     September, 2015
*
* @section  LICENSE
*
* Copyright (C) 2014, Martin Henfling and Daniel Knobl. All rights reserved.
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
* @brief    Implementation of spectrogram class.
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "spectrogram.h"
#include "math.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/SparseCore>
#include <unsupported/Eigen/FFT>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

VectorXd Spectrogram::gauss_window (qint32 sample_count, qreal scale, quint32 translation)
{
    VectorXd gauss = VectorXd::Zero(sample_count);

    for(qint32 n = 0; n < sample_count; n++)
    {
        qreal t = (qreal(n) - translation) / scale;
        gauss[n] = exp(-3.14 * pow(t, 2))*pow(sqrt(scale),(-1))*pow(qreal(2),(0.25));
    }

    return gauss;
}

//-----------------------------------------------------------------------------------------------------------------

MatrixXd Spectrogram::make_spectrogram(VectorXd signal, qint32 window_size = 0)
{
    if(window_size == 0)
        window_size = signal.rows()/4;

    Eigen::FFT<double> fft;
    MatrixXd tf_matrix = MatrixXd::Zero(signal.rows()/2, signal.rows());

    for(qint32 translate = 0; translate < signal.rows(); translate++)
    {
        VectorXd envelope = gauss_window(signal.rows(), window_size, translate);

        VectorXd windowed_sig = VectorXd::Zero(signal.rows());
        VectorXcd fft_win_sig = VectorXcd::Zero(signal.rows());

        VectorXd real_coeffs = VectorXd::Zero(signal.rows()/2);

        for(qint32 sample = 0; sample < signal.rows(); sample++)
            windowed_sig[sample] = signal[sample] * envelope[sample];

        fft.fwd(fft_win_sig, windowed_sig);

        for(qint32 i= 0; i<signal.rows()/2; i++)
        {
            qreal value = pow(std::abs(fft_win_sig[i]), 2.0);
            real_coeffs[i] = value;
        }

        tf_matrix.col(translate) = real_coeffs;
    }
    return tf_matrix;
}
