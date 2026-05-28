//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     cosinefilter.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Definition of the CosineFilter class
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "cosinefilter.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <algorithm>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//#ifndef EIGEN_FFTW_DEFAULT
//#define EIGEN_FFTW_DEFAULT
//#endif

#include <unsupported/Eigen/FFT>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CosineFilter::CosineFilter()
: m_iFilterOrder(0)
{
}

//=============================================================================================================

CosineFilter::CosineFilter(int fftLength,
                           float lowpass,
                           float lowpass_width,
                           float highpass,
                           float highpass_width,
                           double sFreq,
                           TPassType type)
{
    #ifdef EIGEN_FFTW_DEFAULT
        fftw_make_planner_thread_safe();
    #endif

    m_iFilterOrder = fftLength;

    int highpasss,lowpasss;
    int highpass_widths,lowpass_widths;
    int k,s,w;
    int resp_size = fftLength/2+1; //Take half because we are not interested in the conjugate complex part of the spectrum

    double pi4 = M_PI/4.0;
    float mult,add,c;

    RowVectorXcd filterFreqResp = RowVectorXcd::Ones(resp_size);

    //Transform frequencies into samples
    highpasss = ((resp_size-1)*highpass)/(0.5*sFreq);
    lowpasss = ((resp_size-1)*lowpass)/(0.5*sFreq);

    lowpass_widths = ((resp_size-1)*lowpass_width)/(0.5*sFreq);
    lowpass_widths = (lowpass_widths+1)/2;

    if (highpass_width > 0.0) {
        highpass_widths = ((resp_size-1)*highpass_width)/(0.5*sFreq);
        highpass_widths  = (highpass_widths+1)/2;
    }
    else
        highpass_widths = 3;

    //Calculate filter freq response - use cosine
    //Build high pass filter
    if(type != LPF) {
        if (highpasss > highpass_widths + 1) {
            w    = highpass_widths;
            mult = 1.0/w;
            add  = 3.0;

            for (k = 0; k < resp_size; k++)
                filterFreqResp(k) = 0.0;

            for (k = -w+1, s = highpasss-w+1; k < w; k++, s++) {
                if (s >= 0 && s < resp_size) {
                    c = cos(pi4*(k*mult+add));
                    filterFreqResp(s) = filterFreqResp(s).real()*c*c;
                }
            }

            for (k = std::max(0, highpasss + w); k < resp_size; ++k) {
                filterFreqResp(k) = 1.0;
            }
        }
    }

    //Build low pass filter
    if(type != HPF) {
        if (lowpass_widths > 0) {
            w    = lowpass_widths;
            mult = 1.0/w;
            add  = 1.0;

            for (k = -w+1, s = lowpasss-w+1; k < w; k++, s++) {
                if (s >= 0 && s < resp_size) {
                    c = cos(pi4*(k*mult+add));
                    filterFreqResp(s) = filterFreqResp(s).real()*c*c;
                }
            }

            for (k = s; k < resp_size; k++)
                filterFreqResp(k) = 0.0;
        }
        else {
            for (k = lowpasss; k < resp_size; k++)
                filterFreqResp(k) = 0.0;
        }
    }

    m_vecFftCoeff = filterFreqResp;

    //Generate windowed impulse response - invert fft coeeficients to time domain
    Eigen::FFT<double> fft;
    fft.SetFlag(fft.HalfSpectrum);

    //invert to time domain and
    fft.inv(m_vecCoeff, filterFreqResp);/*
    m_vecCoeff = m_vecCoeff.segment(0,1024).eval();

    //window/zero-pad m_vecCoeff to m_iFftLength
    RowVectorXd vecCoeffZeroPad = RowVectorXd::Zero(fftLength);
    vecCoeffZeroPad.head(m_vecCoeff.cols()) = m_vecCoeff;

    //fft-transform filter coeffs
    m_vecFftCoeff = RowVectorXcd::Zero(fftLength);
    fft.fwd(m_vecFftCoeff,vecCoeffZeroPad);*/
}

//=============================================================================================================
