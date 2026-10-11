//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     filteroperator.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @date     February, 2014
 * @version  2.1.0
 * @brief    Contains all FilterOperators.
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filteroperator.h"

#include <algorithm>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FilterOperator::FilterOperator()
: MNEOperator(OperatorType::FILTER)
, m_designMethod()
, m_Type(FilterType::HPF)
, m_sFreq(1024)
, m_iFilterOrder(512)
, m_iFFTlength(512)
, m_dCenterFreq(40)
, m_dBandwidth(3)
{
}


//*************************************************************************************************************

FilterOperator::FilterOperator(QString unique_name, FilterType type, int order, double centerfreq, double bandwidth, double parkswidth, double sFreq, qint32 fftlength, DesignMethod designMethod)
: MNEOperator(OperatorType::FILTER)
, m_designMethod(designMethod)
, m_Type(type)
, m_sFreq(sFreq)
, m_iFilterOrder(order)
, m_iFFTlength(fftlength)
, m_dCenterFreq(centerfreq)
, m_dBandwidth(bandwidth)
, m_sName(unique_name)
{
    // Frequencies are relative to Nyquist. The kernel is linear phase with a delay of half its length.
    const UTILSLIB::FilterKernel kernel(unique_name,
                                        type,
                                        order,
                                        centerfreq,
                                        bandwidth,
                                        parkswidth,
                                        sFreq,
                                        designMethod == Cosine ? 0 : 1);
    m_dCoeffA = kernel.getCoefficients();
    m_iFilterOrder = static_cast<int>(m_dCoeffA.cols());

    //fft-transform m_dCoeffA in order to be able to perform frequency-domain filtering
    fftTransformCoeffs();
}


//*************************************************************************************************************

FilterOperator::~FilterOperator()
{
}


//*************************************************************************************************************

void FilterOperator::fftTransformCoeffs()
{
#ifdef EIGEN_FFTW_DEFAULT
    fftw_make_planner_thread_safe();
#endif

    //This function only nneds to be called when using the Tschebyscheff design method
    //zero-pad m_dCoeffA to m_iFFTlength
    RowVectorXd t_coeffAzeroPad = RowVectorXd::Zero(m_iFFTlength);
    t_coeffAzeroPad.head(m_dCoeffA.cols()) = m_dCoeffA;

    //generate fft object
    Eigen::FFT<double> fft;
    fft.SetFlag(fft.HalfSpectrum);

    //fft-transform filter coeffs
    m_dFFTCoeffA = RowVectorXcd::Zero(m_iFFTlength);
    fft.fwd(m_dFFTCoeffA, t_coeffAzeroPad);
}


//*************************************************************************************************************

RowVectorXd FilterOperator::applyFFTFilter(const RowVectorXd& data) const
{
#ifdef EIGEN_FFTW_DEFAULT
    fftw_make_planner_thread_safe();
#endif

    if (data.size() == 0 || m_dCoeffA.size() == 0) {
        return data;
    }

    const int coeffLength = static_cast<int>(m_dCoeffA.cols());
    const int minFftLength = std::max(m_iFFTlength, static_cast<int>(data.cols()) + coeffLength);

    int fftLength = 1;
    while (fftLength < minFftLength) {
        fftLength <<= 1;
    }

    //generate fft object
    Eigen::FFT<double> fft;
    fft.SetFlag(fft.HalfSpectrum);

    // Transform coefficients on demand when the browser block is longer than the
    // legacy raw-model window size. This keeps the filter path safe for QRHI
    // blocks without forcing the whole app back to the compatibility model.
    RowVectorXcd fftCoefficients;
    if (m_dFFTCoeffA.cols() == fftLength / 2 + 1) {
        fftCoefficients = m_dFFTCoeffA;
    } else {
        RowVectorXd coeffZeroPad = RowVectorXd::Zero(fftLength);
        coeffZeroPad.head(coeffLength) = m_dCoeffA;
        fft.fwd(fftCoefficients, coeffZeroPad, fftLength);
    }

    RowVectorXd paddedData = data;
    if (paddedData.cols() < fftLength) {
        const int residual = fftLength - static_cast<int>(paddedData.cols());
        paddedData.conservativeResize(fftLength);
        paddedData.tail(residual).setZero();
    }

    //fft-transform data sequence
    RowVectorXcd t_freqData;
    fft.fwd(t_freqData, paddedData, fftLength);

    //perform frequency-domain filtering
    RowVectorXcd t_filteredFreq = fftCoefficients.array() * t_freqData.array();

    //inverse-FFT
    RowVectorXd t_filteredTime;
    fft.inv(t_filteredTime, t_filteredFreq);

    const int groupDelay = coeffLength / 2;
    if (groupDelay + data.cols() <= t_filteredTime.cols()) {
        return t_filteredTime.segment(groupDelay, data.cols()).eval();
    }

    return t_filteredTime.head(data.cols()).eval();
}
