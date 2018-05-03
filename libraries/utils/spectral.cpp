//=============================================================================================================
/**
* @file     spectral.cpp
* @author   Daniel Strohmeier <daniel.strohmeier@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Daniel Strohmeier and Matti Hamalainen. All rights reserved.
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
* @note Notes:
* - Some of this code was adapted from mne-python (https://martinos.org/mne) with permission from Alexandre Gramfort.
* - This code is prepared for adding spectral estimation with multitapers, which is, however not yet supported.
* - This code only allows FFT based spectral estimation. Time-frequency transforms are not yet supported.
*
* @brief    Declaration of Spectral class.
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "spectral.h"
#include "math.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <unsupported/Eigen/FFT>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QtMath>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MatrixXcd Spectral::computeTaperedSpectra(const RowVectorXd &vecData,
                                          const MatrixXd &matTaper,
                                          int iNfft)
{
    FFT<double> fft;
    fft.SetFlag(fft.HalfSpectrum);

    //Check inputs
    if (vecData.cols() != matTaper.cols()) {
        return MatrixXcd();
    }
    if (iNfft < vecData.cols()) {
        return MatrixXcd();
    }

    //FFT for freq domain returning the half spectrum
    RowVectorXd vecInputFFT;
    RowVectorXcd vecTmpFreq;
    MatrixXcd matTapSpectrum(matTaper.rows(), int(floor(iNfft / 2.0)) + 1);
    for (int i=0; i < matTaper.rows(); i++) {
        vecInputFFT = vecData.cwiseProduct(matTaper.row(i));
        fft.fwd(vecTmpFreq, vecInputFFT, iNfft);
        matTapSpectrum.row(i) = vecTmpFreq;
    }

    return matTapSpectrum;
}


//*************************************************************************************************************

RowVectorXd Spectral::psdFromTaperedSpectra(const MatrixXcd &matTapSpectrum,
                                            const VectorXd &vecTapWeights,
                                            int iNfft,
                                            double dSampFreq)
{
    //Check inputs
    if (matTapSpectrum.rows() != vecTapWeights.rows()) {
        return RowVectorXd();
    }

    //Compute PSD (average over tapers if necessary)
    double denom = vecTapWeights.cwiseAbs2().sum();  //We use fix weights (adaptive weights not yet supported)
    RowVectorXd vecPsd = (vecTapWeights.asDiagonal() * matTapSpectrum).cwiseAbs2().colwise().sum() / denom;

    //multiply by 2 due to half spectrum
    vecPsd *= 2.0;
    vecPsd(0) /= 2.0;
    if (iNfft % 2 == 0){
        vecPsd.tail(1) /= 2.0;
    }

    //Normalization
    vecPsd /= dSampFreq;

    return vecPsd;
}


//*************************************************************************************************************

RowVectorXcd Spectral::csdFromTaperedSpectra(const MatrixXcd &vecTapSpectrumSeed,
                                             const MatrixXcd &vecTapSpectrumTarget,
                                             const VectorXd &vecTapWeights,
                                             int iNfft,
                                             double dSampFreq)
{
    //Check inputs
    if (vecTapSpectrumSeed.rows() != vecTapSpectrumTarget.rows()) {
        return MatrixXcd();
    }
    if (vecTapSpectrumSeed.cols() != vecTapSpectrumTarget.cols()) {
        return MatrixXcd();
    }
    if (vecTapSpectrumSeed.rows() != vecTapWeights.rows()) {
        return MatrixXcd();
    }

    //Compute PSD (average over tapers if necessary)
    double denom = vecTapWeights.cwiseAbs2().sum();    //We use fix weights (adaptive weights not yet supported)
    RowVectorXcd vecCsd = (vecTapWeights.asDiagonal() * vecTapSpectrumSeed).cwiseProduct((vecTapWeights.asDiagonal() * vecTapSpectrumTarget).conjugate()).colwise().sum() / denom;

    //multiply by 2 due to half spectrum
    vecCsd *= 2.0;
    vecCsd(0) /= 2.0;
    if (iNfft % 2 == 0){
        vecCsd.tail(1) /= 2.0;
    }

    //Normalization
    vecCsd /= dSampFreq;

    return vecCsd;
}


//*************************************************************************************************************

VectorXd Spectral::calculateFFTFreqs(int iNfft, double dSampFreq)
{
    //Compute FFT frequencies
    RowVectorXd vecFFTFreqs;
    if (iNfft % 2 == 0){
        vecFFTFreqs = (dSampFreq / iNfft) * RowVectorXd::LinSpaced(iNfft / 2.0 + 1, 0.0, iNfft / 2.0);
    } else {
        vecFFTFreqs = (dSampFreq / iNfft) * RowVectorXd::LinSpaced((iNfft - 1) / 2.0 + 1, 0.0, (iNfft - 1) / 2.0);
    }
    return vecFFTFreqs;
}


//*************************************************************************************************************

QPair<MatrixXd, VectorXd> Spectral::generateTapers(int iSignalLength, const QString &sWindowType)
{
    QPair<MatrixXd, VectorXd> pairOut;
    if (sWindowType == "hanning") {
        pairOut.first = hanningWindow(iSignalLength);
        pairOut.second = VectorXd::Ones(1);
    } else if (sWindowType == "ones") {
        pairOut.first = MatrixXd::Ones(1, iSignalLength) / double(iSignalLength);
        pairOut.second = VectorXd::Ones(1);
    } else {
        pairOut.first = hanningWindow(iSignalLength);
        pairOut.second = VectorXd::Ones(1);
    }
    return pairOut;
}


//*************************************************************************************************************

MatrixXd Spectral::hanningWindow(int iSignalLength)
{
    MatrixXd matHann = MatrixXd::Zero(1, iSignalLength);

    //Main step of building the hanning window
    for (int n = 0; n < iSignalLength; n++) {
        matHann(0, n) = 0.5 - 0.5 * cos(2.0 * M_PI * n / (iSignalLength - 1.0));
    }
    matHann.array() /= matHann.row(0).norm();

    return matHann;
}
