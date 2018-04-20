//=============================================================================================================
/**
* @file     spectral.h
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
* Notes:
* - Some of this code was adapted from mne-python (https://martinos.org/mne) with permission from Alexandre Gramfort.
* - This code is prepared for adding spectral estimation with multitapers, which is, however not yet supported.
* - This code only allows FFT based spectral estimation. Time-frequency transforms are not yet supported.
*
* @brief    Declaration of Spectral class.
*/

#ifndef SPECTRAL_H
#define SPECTRAL_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils_global.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QString>
#include <QPair>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{
//=============================================================================================================
/**
* Computes spectral measures of input data such as spectra, power spectral density, cross-spectral density.
*
* @brief Computes spectral measures of input data.
*/
class UTILSSHARED_EXPORT Spectral
{

public:
    //=========================================================================================================
    /**
    * deleted default constructor (static class).
    */
    Spectral() = delete;

    //=========================================================================================================
    /**
    * Calculates the full tapered spectra of a given input data
    *
    * @param[in] vecData         input data (time domain), for which the spectrum is computed
    * @param[in] matTaper        tapers used to compute the spectra
    * @param[in] iNfft           FFT length
    *
    * @return tapered spectra of the input data
    */
    static Eigen::MatrixXcd computeTaperedSpectra(const Eigen::VectorXd &vecData,
                                                  const Eigen::MatrixXd &matTaper,
                                                  int iNfft);

    //=========================================================================================================
    /**
    * Calculates the FFT frequencies
    *
    * @param[in] iNfft            FFT length
    * @param[in] dSampFreq        sampling frequency of the input data
    *
    * @return FFT frequencies
    */
    static Eigen::VectorXd calculateFFTFreqs(int iNfft, double dSampFreq);

    //=========================================================================================================
    /**
    * Calculates a hanning window of given length
    *
    * @param[in] iSignalLength    length of the hanning window
    * @param[in] sWindowType      type of the window function used to compute tapered spectra
    *
    * @return Qpair of tapers and taper weights
    */
    static QPair<Eigen::MatrixXd, Eigen::VectorXd> generateTapers(int iSignalLength,
                                                                  const QString &sWindowType = "hanning");

private:
    //=========================================================================================================
    /**
    * Calculates a hanning window of given length
    *
    * @param[in] iSignalLength     length of the hanning window
    *
    * @return hanning window
    */
    static Eigen::MatrixXd hanningWindow(int iSignalLength);

};

}//namespace

#endif // SPECTRAL_H
