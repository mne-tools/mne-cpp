//=============================================================================================================
/**
* @file     filteroperator.h
* @author   Florian Schlembach <florian.schlembach@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           Jens Haueisen <jens.haueisen@tu-ilmenau.de>
* @version  1.0
* @date     February, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Florian Schlembach, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains all FilterOperators.
*
*/

#ifndef FILTEROPERATOR_H
#define FILTEROPERATOR_H

//=============================================================================================================
// INCLUDES

//MNE
#include "mneoperator.h"
#include <fiff/fiff.h>
#include <mne/mne.h>
#include <utils/parksmcclellan.h>


//Eigen
#include <Eigen/Core>
#include <Eigen/SparseCore>
#include <Eigen/unsupported/FFT>

#ifndef EIGEN_FFTW_DEFAULT
#define EIGEN_FFTW_DEFAULT
#endif

//=============================================================================================================
// NAMESPACES

using namespace MNELIB;
using namespace Eigen;

//*************************************************************************************************************

class FilterOperator : public MNEOperator
{
public:
    enum FilterType {
       LPF,
       HPF,
       BPF,
       NOTCH
    } m_Type;

    FilterOperator();

    FilterOperator(FilterType type, qint8 order, double centerfreq, double bandwidth, double parkswidth, qint32 fftlength=4096);

    qint8 m_iFilterOrder; /**< represents the order of the filter instance */
    qint32 m_iFFTlength; /**< represents the filter length */

    RawVectorXd m_dCoeffA; /**< contains the forward filter coefficient set */
    RawVectorXd m_dCoeffB; /**< contains the backward filter coefficient set (empty if FIR filter) */

    RawVectorXd m_dFFTCoeffA; /**< the FFT-transformed forward filter coefficient set, required for frequency-domain filtering, zero-padded to m_iFFTlength */
    RawVectorXd m_dFFTCoeffB; /**< the FFT-transformed backward filter coefficient set, required for frequency-domain filtering, zero-padded to m_iFFTlength */
};

#endif // FILTEROPERATOR_H
