//=============================================================================================================
/**
 * @file     filteroperator.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     February, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    The FilterOperator class represents a derived class from an arbitrary MNEOperator class object.
 *           Thus, it is a filter object that generates the FIR filter coefficients using Park's McClellan's
 *           filter design algorithm [1] and offers a overlap-add method [2] for frequency filtering of an input
 *           sequence. In this regard, the filter coefficients of a certain filter order are zero-padded to fill
 *           a length of an multiple integer of a power of 2 in order to efficiently compute a FFT. The length of
 *           the FFT is given by the next power of 2 of the length of the input sequence. In order to avoid
 *           circular-convolution, the input sequence is given by the FFT-length-NumFilterTaps.
 *
 *           e.g. FFT length=4096, NumFilterTaps=80 -> input sequence 4096-80=4016
 *
 *
 *           [1] http://en.wikipedia.org/wiki/Parks%E2%80%93McClellan_filter_design_algorithm
 *           [2] http://en.wikipedia.org/wiki/Overlap_add
 */
#ifndef FILTEROPERATOR_H
#define FILTEROPERATOR_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp/viewers/helpers/mneoperator.h>
#include "types.h"


//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <mne/mne.h>
#include <utils/filterTools/parksmcclellan.h>
#include <utils/filterTools/cosinefilter.h>
#include <disp/viewers/helpers/mneoperator.h>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>
#include <unsupported/Eigen/FFT>

//#ifndef EIGEN_FFTW_DEFAULT
//#define EIGEN_FFTW_DEFAULT
//#endif


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace Eigen;
using namespace UTILSLIB;
using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{

//=============================================================================================================
/**
 * DECLARE CLASS FilterOperator
 */
class FilterOperator : public MNEOperator
{
public:
    enum DesignMethod {
       Tschebyscheff,
       Cosine} m_designMethod;

    enum FilterType {
       LPF,
       HPF,
       BPF,
       NOTCH
    } m_Type;

    FilterOperator();

    //=========================================================================================================
    /**
    * FilterOperator::FilterOperator
    *
    * @param unique_name defines the name of the generated filter
    * @param type of the filter: LPF, HPF, BPF, NOTCH (from enum FilterType)
    * @param order represents the order of the filter, the higher the higher is the stopband attenuation
    * @param centerfreq determines the center of the frequency
    * @param bandwidth ignored if FilterType is set to LPF,HPF. if NOTCH/BPF: bandwidth of stop-/passband
    * @param parkswidth determines the width of the filter slopes (steepness)
    * @param sFreq sampling frequency
    * @param fftlength length of the fft (multiple integer of 2^x)
    */
    FilterOperator(QString unique_name, FilterType type, int order, double centerfreq, double bandwidth, double parkswidth, double sFreq, qint32 fftlength=4096, DesignMethod method = Cosine);

    //=========================================================================================================
    /**
    * Destructor
    */
    ~FilterOperator();

    //=========================================================================================================
    /**
    * Transforms the calculated filter coefficients to frequency-domain
    */
    void fftTransformCoeffs();

    //=========================================================================================================
    /**
    * FilterOperator::FilterOperator
    *
    * @param data the input data which is to be filtered
    * @return a row vector truncated by numberFilterTaps/2 at front and end
    */
    RowVectorXd applyFFTFilter(const RowVectorXd& data) const;

    double          m_sFreq;            /**< the sampling frequency. */
    int             m_iFilterOrder;     /**< represents the order of the filter instance. */
    int             m_iFFTlength;       /**< represents the filter length. */
    double          m_dCenterFreq;      /**< contains center freq of the filter. */
    double          m_dBandwidth;       /**< contains bandwidth of the filter. */

    QString         m_sName;            /**< contains name of the filter. */

    RowVectorXd     m_dCoeffA;          /**< contains the forward filter coefficient set. */
    RowVectorXd     m_dCoeffB;          /**< contains the backward filter coefficient set (empty if FIR filter). */

    RowVectorXcd    m_dFFTCoeffA;       /**< the FFT-transformed forward filter coefficient set, required for frequency-domain filtering, zero-padded to m_iFFTlength. */
    RowVectorXcd    m_dFFTCoeffB;       /**< the FFT-transformed backward filter coefficient set, required for frequency-domain filtering, zero-padded to m_iFFTlength. */
};

} // NAMESPACE

#endif // FILTEROPERATOR_H
