//=============================================================================================================
/**
 * @file     filterkernel.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Ruben Doerfel <Ruben.Doerfel@tu-ilmenau.de>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Ruben Doerfel, Christoph Dinh. All rights reserved.
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
 * @brief    Contains all FilterKernel.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filterkernel.h"

#include <utils/mnemath.h>

#include "parksmcclellan.h"
#include "cosinefilter.h"

#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/SparseCore>
//#ifndef EIGEN_FFTW_DEFAULT
//#define EIGEN_FFTW_DEFAULT
//#endif
#include <unsupported/Eigen/FFT>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTPROCESSINGLIB;
using namespace Eigen;
using namespace UTILSLIB;

//=============================================================================================================

FilterKernel::FilterKernel()
: m_Type(UNKNOWN)
, m_iFilterOrder(80)
, m_sFilterName("Unknown")
, m_dParksWidth(0.1)
, m_designMethod(External)
, m_dCenterFreq(0.5)
, m_dBandwidth(0.1)
, m_sFreq(1000)
, m_dLowpassFreq(4)
, m_dHighpassFreq(40)
{
    designFilter();
}

//=============================================================================================================

FilterKernel::FilterKernel(const QString& sFilterName,
                           FilterType type,
                           int iOrder,
                           double dCenterfreq,
                           double dBandwidth,
                           double dParkswidth,
                           double dSFreq,
                           DesignMethod designMethod)
: m_designMethod(designMethod)
, m_Type(type)
, m_sFreq(dSFreq)
, m_dCenterFreq(dCenterfreq)
, m_dBandwidth(dBandwidth)
, m_dParksWidth(dParkswidth)
, m_iFilterOrder(iOrder)
, m_sFilterName(sFilterName)
{
    if(iOrder < 9) {
       qWarning() << "[FilterKernel::FilterKernel] Less than 9 taps were provided. Setting number of taps to 9.";
    }

    designFilter();
}

//=============================================================================================================

bool FilterKernel::fftTransformCoeffs(int iFftLength)
{
    #ifdef EIGEN_FFTW_DEFAULT
        fftw_make_planner_thread_safe();
    #endif

    if(m_vecCoeff.cols() > iFftLength) {
        std::cout <<"[FilterKernel::fftTransformCoeffs] The number of filter taps is bigger than the FFT length."<< std::endl;
        return false;
    }

    //generate fft object
    Eigen::FFT<double> fft;
    fft.SetFlag(fft.HalfSpectrum);

    // Zero padd if necessary. Please note: The zero padding in Eigen's FFT is only working for column vectors -> We have to zero pad manually here
    RowVectorXd vecInputFft;
    if (m_vecCoeff.cols() < iFftLength) {
        vecInputFft.setZero(iFftLength);
        vecInputFft.block(0,0,1,m_vecCoeff.cols()) = m_vecCoeff;
    } else {
        vecInputFft = m_vecCoeff;
    }

    //fft-transform filter coeffs
    RowVectorXcd vecFreqData;
    fft.fwd(vecFreqData, vecInputFft, iFftLength);
    m_vecFftCoeff = vecFreqData;;

    return true;
}

//=============================================================================================================

RowVectorXd FilterKernel::applyConvFilter(const RowVectorXd& vecData,
                                          bool bKeepOverhead) const
{
    //Do zero padding or mirroring depending on user input
    RowVectorXd vecDataZeroPad = RowVectorXd::Zero(2*m_vecCoeff.cols() + vecData.cols());
    RowVectorXd vecFilteredTime = RowVectorXd::Zero(2*m_vecCoeff.cols() + vecData.cols());

    vecDataZeroPad.segment(m_vecCoeff.cols(), vecData.cols()) = vecData;

    //Do the convolution
    for(int i = m_vecCoeff.cols(); i < vecFilteredTime.cols(); i++) {
        vecFilteredTime(i-m_vecCoeff.cols()) = vecDataZeroPad.segment(i-m_vecCoeff.cols(),m_vecCoeff.cols()) * m_vecCoeff.transpose();
    }

    //Return filtered data
    if(!bKeepOverhead) {
        return vecFilteredTime.segment(m_vecCoeff.cols()/2, vecData.cols());
    }

    return vecFilteredTime.head(vecData.cols()+m_vecCoeff.cols());
}

//=============================================================================================================

RowVectorXd FilterKernel::applyFftFilter(const RowVectorXd& vecData,
                                         bool bKeepOverhead)
{
    #ifdef EIGEN_FFTW_DEFAULT
    fftw_make_planner_thread_safe();
    #endif

    // Make sure we always have the correct FFT length for the given input data and filter overlap
    int iFftLength = vecData.cols() + m_vecCoeff.cols();
    int exp = ceil(MNEMath::log2(iFftLength));
    iFftLength = pow(2, exp);

    // Transform coefficients anew if needed
    if(m_vecFftCoeff.cols() != (iFftLength/2+1)) {
        fftTransformCoeffs(iFftLength);
    }

    //generate fft object
    Eigen::FFT<double> fft;
    fft.SetFlag(fft.HalfSpectrum);

    // Zero padd if necessary. Please note: The zero padding in Eigen's FFT is only working for column vectors -> We have to zero pad manually here
    RowVectorXd vecInputFft;
    if (vecData.cols() < iFftLength) {
        vecInputFft.setZero(iFftLength);
        vecInputFft.block(0,0,1,vecData.cols()) = vecData;
    } else {
        vecInputFft = vecData;
    }

    //fft-transform data sequence
    RowVectorXcd vecFreqData;
    fft.fwd(vecFreqData, vecInputFft, iFftLength);

    //perform frequency-domain filtering
    RowVectorXcd vecFilteredFreq = m_vecFftCoeff.array() * vecFreqData.array();

    //inverse-FFT
    RowVectorXd vecFilteredTime;
    fft.inv(vecFilteredTime, vecFilteredFreq);

    //Return filtered data
    if(!bKeepOverhead) {
        return vecFilteredTime.segment(m_vecCoeff.cols()/2, vecData.cols());
    }

    return vecFilteredTime.head(vecData.cols()+m_vecCoeff.cols());
}

//=============================================================================================================

QString FilterKernel::getStringForDesignMethod(const FilterKernel::DesignMethod &designMethod)
{
    QString designMethodString = "External";

    if(designMethod == FilterKernel::External)
        designMethodString = "External";

    if(designMethod == FilterKernel::Cosine)
        designMethodString = "Cosine";

    if(designMethod == FilterKernel::Tschebyscheff)
        designMethodString = "Tschebyscheff";

    return designMethodString;
}

//=============================================================================================================

QString FilterKernel::getStringForFilterType(const FilterKernel::FilterType &filterType)
{
    QString filterTypeString = "LPF";

    if(filterType == FilterKernel::LPF)
        filterTypeString = "LPF";

    if(filterType == FilterKernel::HPF)
        filterTypeString = "HPF";

    if(filterType == FilterKernel::BPF)
        filterTypeString = "BPF";

    if(filterType == FilterKernel::NOTCH)
        filterTypeString = "NOTCH";

    return filterTypeString;
}

//=============================================================================================================

FilterKernel::DesignMethod FilterKernel::getDesignMethodForString(const QString &designMethodString)
{
    FilterKernel::DesignMethod designMethod = FilterKernel::External;

    if(designMethodString == "External")
        designMethod = FilterKernel::External;

    if(designMethodString == "Tschebyscheff")
        designMethod = FilterKernel::Tschebyscheff;

    if(designMethodString == "Cosine")
        designMethod = FilterKernel::Cosine;

    return designMethod;
}

//=============================================================================================================

FilterKernel::FilterType FilterKernel::getFilterTypeForString(const QString &filterTypeString)
{
    FilterKernel::FilterType filterType = FilterKernel::UNKNOWN;

    if(filterTypeString == "LPF")
        filterType = FilterKernel::LPF;

    if(filterTypeString == "HPF")
        filterType = FilterKernel::HPF;

    if(filterTypeString == "BPF")
        filterType = FilterKernel::BPF;

    if(filterTypeString == "NOTCH")
        filterType = FilterKernel::NOTCH;

    return filterType;
}

//=============================================================================================================

void FilterKernel::prepareFilter(FilterKernel& filterKernel,
                                 int iDataSize)
{
    int iFftLength, exp;

    iFftLength = iDataSize + filterKernel.getCoefficients().cols();
    exp = ceil(MNEMath::log2(iFftLength));
    iFftLength = pow(2, exp);

    // Transform coefficients anew if needed
    if(filterKernel.getFftCoefficients().cols() != (iFftLength/2+1)) {
        filterKernel.fftTransformCoeffs(iFftLength);
    }
}

//=============================================================================================================

QString FilterKernel::getName() const
{
    return m_sFilterName;
}

//=============================================================================================================

void FilterKernel::setName(const QString& sFilterName)
{
    m_sFilterName = sFilterName;
}

//=============================================================================================================

double FilterKernel::getSamplingFrequency() const
{
    return m_sFreq;
}

//=============================================================================================================

void FilterKernel::setSamplingFrequency(double dSFreq)
{
    m_sFreq = dSFreq;
}

//=============================================================================================================

int FilterKernel::getFilterOrder() const
{
    return m_iFilterOrder;
}

//=============================================================================================================

void FilterKernel::setFilterOrder(int iOrder)
{
    m_iFilterOrder = iOrder;
}

//=============================================================================================================

double FilterKernel::getCenterFrequency() const
{
    return m_dCenterFreq;
}

//=============================================================================================================

void FilterKernel::setCenterFrequency(double dCenterFreq)
{
    m_dCenterFreq = dCenterFreq;
}

//=============================================================================================================

double FilterKernel::getBandwidth() const
{
    return m_dBandwidth;
}

//=============================================================================================================

void FilterKernel::setBandwidth(double dBandwidth)
{
    m_dBandwidth = dBandwidth;
}

//=============================================================================================================

double FilterKernel::getParksWidth() const
{
    return m_dParksWidth;
}

//=============================================================================================================

void FilterKernel::setParksWidth(double dParksWidth)
{
    m_dParksWidth = dParksWidth;
}

//=============================================================================================================

double FilterKernel::getHighpassFreq() const
{
    return m_dHighpassFreq;
}

//=============================================================================================================

void FilterKernel::setHighpassFreq(double dHighpassFreq)
{
    m_dHighpassFreq = dHighpassFreq;
}

//=============================================================================================================

double FilterKernel::getLowpassFreq() const
{
    return m_dLowpassFreq;
}

//=============================================================================================================

void FilterKernel::setLowpassFreq(double dLowpassFreq)
{
    m_dLowpassFreq = dLowpassFreq;
}

//=============================================================================================================

Eigen::RowVectorXd FilterKernel::getCoefficients() const
{
    return m_vecCoeff;
}

//=============================================================================================================

void FilterKernel::setCoefficients(const Eigen::RowVectorXd& vecCoeff)
{
    m_vecCoeff = vecCoeff;
}

//=============================================================================================================

Eigen::RowVectorXcd FilterKernel::getFftCoefficients() const
{
    return m_vecFftCoeff;
}

//=============================================================================================================

void FilterKernel::setFftCoefficients(const Eigen::RowVectorXcd& vecFftCoeff)
{
    m_vecFftCoeff = vecFftCoeff;
}

//=============================================================================================================

void FilterKernel::designFilter()
{
    // Make sure we only use a minimum needed FFT size
    int iFftLength = m_iFilterOrder;
    int exp = ceil(MNEMath::log2(iFftLength));
    iFftLength = pow(2, exp);

    switch(m_designMethod) {
        case Tschebyscheff: {
            ParksMcClellan filter(m_iFilterOrder,
                                  m_dCenterFreq,
                                  m_dBandwidth,
                                  m_dParksWidth,
                                  static_cast<ParksMcClellan::TPassType>(m_Type));
            m_vecCoeff = filter.FirCoeff;

            //fft-transform m_vecCoeff in order to be able to perform frequency-domain filtering
            fftTransformCoeffs(iFftLength);

            break;
        }

        case Cosine: {
            CosineFilter filtercos;

            switch(m_Type) {
                case LPF:
                    filtercos = CosineFilter(iFftLength,
                                             (m_dCenterFreq)*(m_sFreq/2.),
                                             m_dParksWidth*(m_sFreq/2),
                                             (m_dCenterFreq)*(m_sFreq/2),
                                             m_dParksWidth*(m_sFreq/2),
                                             m_sFreq,
                                             (CosineFilter::TPassType)m_Type);

                    break;

                case HPF:
                    filtercos = CosineFilter(iFftLength,
                                             (m_dCenterFreq)*(m_sFreq/2),
                                             m_dParksWidth*(m_sFreq/2),
                                             (m_dCenterFreq)*(m_sFreq/2),
                                             m_dParksWidth*(m_sFreq/2),
                                             m_sFreq,
                                             (CosineFilter::TPassType)m_Type);

                    break;

                case BPF:
                    filtercos = CosineFilter(iFftLength,
                                             (m_dCenterFreq + m_dBandwidth/2)*(m_sFreq/2),
                                             m_dParksWidth*(m_sFreq/2),
                                             (m_dCenterFreq - m_dBandwidth/2)*(m_sFreq/2),
                                             m_dParksWidth*(m_sFreq/2),
                                             m_sFreq,
                                             (CosineFilter::TPassType)m_Type);

                    break;
            }

            //This filter is designed in the frequency domain, hence the time domain impulse response need to be shortend by the users dependent number of taps
            m_vecCoeff.resize(m_iFilterOrder);

            m_vecCoeff.head(m_iFilterOrder/2) = filtercos.m_vecCoeff.tail(m_iFilterOrder/2);
            m_vecCoeff.tail(m_iFilterOrder/2) = filtercos.m_vecCoeff.head(m_iFilterOrder/2);

            //Now generate the fft version of the shortened impulse response
            fftTransformCoeffs(iFftLength);

            break;
        }
    }

    switch(m_Type) {
        case LPF:
            m_dLowpassFreq = 0;
            m_dHighpassFreq = m_dCenterFreq*(m_sFreq/2);
        break;

        case HPF:
            m_dLowpassFreq = m_dCenterFreq*(m_sFreq/2);
            m_dHighpassFreq = 0;
        break;

        case BPF:
            m_dLowpassFreq = (m_dCenterFreq + m_dBandwidth/2)*(m_sFreq/2);
            m_dHighpassFreq = (m_dCenterFreq - m_dBandwidth/2)*(m_sFreq/2);
        break;
    }
}
