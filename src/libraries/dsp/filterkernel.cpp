//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     filterkernel.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Florian Schlembach <fschlembach@web.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Jens Haueisen <jens.haueisen@tu-ilmenau.de>;
 *           Ruben Doerfel <doerfelruben@aol.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Felix Arndt <Felix.Arndt@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February 2014
 * @brief    Contains all FilterKernel.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filterkernel.h"
#include <base/log.h>

#include <vector>

#include <math/numerics.h>

#include "parksmcclellan.h"
#include "cosinefilter.h"

#include <algorithm>
#include <format>
#include <utility>

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

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// INIT STATIC MEMBERS
//=============================================================================================================

const std::vector<mne::FilterParameter> FilterKernel::design_methods({FilterParameter("Cosine", "A cosine filter"),
                                                                      FilterParameter("Tschebyscheff", "A tschebyscheff filter")});
const std::vector<mne::FilterParameter> FilterKernel::filter_types({FilterParameter("LPF", "An LPF filter"),
                                                                    FilterParameter("HPF", "An HPF filter"),
                                                                    FilterParameter("BPF", "A BPF filter"),
                                                                    FilterParameter("NOTCH", "A NOTCH filter"),
                                                                    FilterParameter("UNKNOWN", "An UNKNOWN filter")});

namespace
{

int indexOfName(const std::vector<FilterParameter>& table, std::string_view sName)
{
    const auto it = std::ranges::find(table, sName, &FilterParameter::name);
    return it == table.end() ? -1 : static_cast<int>(it - table.begin());
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FilterKernel::FilterKernel()
: m_sFreq(1000)
, m_dCenterFreq(0.5)
, m_dBandwidth(0.1)
, m_dParksWidth(0.1)
, m_dLowpassFreq(40)
, m_dHighpassFreq(4)
, m_iFilterOrder(80)
, m_iDesignMethod(design_method_index("Cosine"))
, m_iFilterType(filter_type_index("BPF"))
, m_sFilterName("Unknown")
{
    design_filter();
}

//=============================================================================================================

FilterKernel::FilterKernel(const std::string& sFilterName,
                           int iFilterType,
                           int iOrder,
                           double dCenterfreq,
                           double dBandwidth,
                           double dParkswidth,
                           double dSFreq,
                           int iDesignMethod)
: m_sFreq(dSFreq)
, m_dCenterFreq(dCenterfreq)
, m_dBandwidth(dBandwidth)
, m_dParksWidth(dParkswidth)
, m_iFilterOrder(iOrder)
, m_iDesignMethod(iDesignMethod)
, m_iFilterType(iFilterType)
, m_sFilterName(sFilterName)
{
    if (iOrder < 9) {
        Log::warning("[FilterKernel::FilterKernel] Less than 9 taps were provided. Setting number of taps to 9.");
    }

    design_filter();
}

//=============================================================================================================

void FilterKernel::prepare_filter(int iDataSize)
{
    int iFftLength, exp;

    iFftLength = iDataSize + m_vecCoeff.cols();
    exp = ceil(Numerics::log2(iFftLength));
    iFftLength = pow(2, exp);

    // Transform coefficients anew if needed
    if (m_vecFftCoeff.cols() != (iFftLength / 2 + 1)) {
        fft_transform_coeffs(iFftLength);
    }
}

//=============================================================================================================

RowVectorXd FilterKernel::apply_conv_filter(const RowVectorXd& vecData,
                                            bool bKeepOverhead) const
{
    //Do zero padding or mirroring depending on user input
    RowVectorXd vecDataZeroPad = RowVectorXd::Zero(2 * m_vecCoeff.cols() + vecData.cols());
    RowVectorXd vecFilteredTime = RowVectorXd::Zero(2 * m_vecCoeff.cols() + vecData.cols());

    vecDataZeroPad.segment(m_vecCoeff.cols(), vecData.cols()) = vecData;

    //Do the convolution
    for (int i = m_vecCoeff.cols(); i < vecFilteredTime.cols(); i++) {
        vecFilteredTime(i - m_vecCoeff.cols()) = vecDataZeroPad.segment(i - m_vecCoeff.cols(), m_vecCoeff.cols()) * m_vecCoeff.transpose();
    }

    //Return filtered data
    if (!bKeepOverhead) {
        return vecFilteredTime.segment(m_vecCoeff.cols() / 2, vecData.cols());
    }

    return vecFilteredTime.head(vecData.cols() + m_vecCoeff.cols());
}

//=============================================================================================================

void FilterKernel::apply_fft_filter(RowVectorXd& vecData,
                                    bool bKeepOverhead)
{
#ifdef EIGEN_FFTW_DEFAULT
    fftw_make_planner_thread_safe();
#endif

    // Make sure we always have the correct FFT length for the given input data and filter overlap
    int iFftLength = vecData.cols() + m_vecCoeff.cols();
    int exp = ceil(Numerics::log2(iFftLength));
    iFftLength = pow(2, exp);

    // Transform coefficients anew if needed
    if (m_vecFftCoeff.cols() != (iFftLength / 2 + 1)) {
        fft_transform_coeffs(iFftLength);
    }

    //generate fft object
    Eigen::FFT<double> fft;
    fft.SetFlag(fft.HalfSpectrum);

    // Zero padd if necessary. Please note: The zero padding in Eigen's FFT is only working for column vectors -> We have to zero pad manually here
    int iOriginalSize = vecData.cols();
    if (vecData.cols() < iFftLength) {
        int iResidual = iFftLength - vecData.cols();
        vecData.conservativeResize(iFftLength);
        vecData.tail(iResidual).setZero();
    }

    //fft-transform data sequence
    RowVectorXcd vecFreqData;
    fft.fwd(vecFreqData, vecData, iFftLength);

    //perform frequency-domain filtering
    vecFreqData = m_vecFftCoeff.array() * vecFreqData.array();

    //inverse-FFT
    fft.inv(vecData, vecFreqData);

    //Return filtered data
    if (!bKeepOverhead) {
        vecData = vecData.segment(m_vecCoeff.cols() / 2, iOriginalSize).eval();
    } else {
        vecData = vecData.head(iOriginalSize + m_vecCoeff.cols()).eval();
    }
}

//=============================================================================================================

const std::string& FilterKernel::name() const
{
    return m_sFilterName;
}

//=============================================================================================================

void FilterKernel::set_name(std::string sFilterName)
{
    m_sFilterName = std::move(sFilterName);
}

//=============================================================================================================

double FilterKernel::sampling_frequency() const
{
    return m_sFreq;
}

//=============================================================================================================

void FilterKernel::set_sampling_frequency(double dSFreq)
{
    m_sFreq = dSFreq;
}

//=============================================================================================================

int FilterKernel::filter_order() const
{
    return m_iFilterOrder;
}

//=============================================================================================================

void FilterKernel::set_filter_order(int iOrder)
{
    m_iFilterOrder = iOrder;
}

//=============================================================================================================

double FilterKernel::center_frequency() const
{
    return m_dCenterFreq;
}

//=============================================================================================================

void FilterKernel::set_center_frequency(double dCenterFreq)
{
    m_dCenterFreq = dCenterFreq;
}

//=============================================================================================================

double FilterKernel::bandwidth() const
{
    return m_dBandwidth;
}

//=============================================================================================================

void FilterKernel::set_bandwidth(double dBandwidth)
{
    m_dBandwidth = dBandwidth;
}

//=============================================================================================================

double FilterKernel::parks_width() const
{
    return m_dParksWidth;
}

//=============================================================================================================

void FilterKernel::set_parks_width(double dParksWidth)
{
    m_dParksWidth = dParksWidth;
}

//=============================================================================================================

double FilterKernel::highpass_freq() const
{
    return m_dHighpassFreq;
}

//=============================================================================================================

void FilterKernel::set_highpass_freq(double dHighpassFreq)
{
    m_dHighpassFreq = dHighpassFreq;
}

//=============================================================================================================

double FilterKernel::lowpass_freq() const
{
    return m_dLowpassFreq;
}

//=============================================================================================================

void FilterKernel::set_lowpass_freq(double dLowpassFreq)
{
    m_dLowpassFreq = dLowpassFreq;
}

//=============================================================================================================

const Eigen::RowVectorXd& FilterKernel::coefficients() const
{
    return m_vecCoeff;
}

//=============================================================================================================

void FilterKernel::set_coefficients(const Eigen::RowVectorXd& vecCoeff)
{
    m_vecCoeff = vecCoeff;
}

//=============================================================================================================

const Eigen::RowVectorXcd& FilterKernel::fft_coefficients() const
{
    return m_vecFftCoeff;
}

//=============================================================================================================

void FilterKernel::set_fft_coefficients(const Eigen::RowVectorXcd& vecFftCoeff)
{
    m_vecFftCoeff = vecFftCoeff;
}

//=============================================================================================================

bool FilterKernel::fft_transform_coeffs(int iFftLength)
{
#ifdef EIGEN_FFTW_DEFAULT
    fftw_make_planner_thread_safe();
#endif

    if (m_vecCoeff.cols() > iFftLength) {
        Log::warning("[FilterKernel::fft_transform_coeffs] The number of filter taps is bigger than the FFT length.");
        return false;
    }

    //generate fft object
    Eigen::FFT<double> fft;
    fft.SetFlag(fft.HalfSpectrum);

    // Zero padd if necessary. Please note: The zero padding in Eigen's FFT is only working for column vectors -> We have to zero pad manually here
    RowVectorXd vecInputFft;
    if (m_vecCoeff.cols() < iFftLength) {
        vecInputFft.setZero(iFftLength);
        vecInputFft.block(0, 0, 1, m_vecCoeff.cols()) = m_vecCoeff;
    } else {
        vecInputFft = m_vecCoeff;
    }

    //fft-transform filter coeffs
    RowVectorXcd vecFreqData;
    fft.fwd(vecFreqData, vecInputFft, iFftLength);
    m_vecFftCoeff = vecFreqData;

    return true;
}

//=============================================================================================================

void FilterKernel::design_filter()
{
    // Make sure we only use a minimum needed FFT size
    int iFftLength = m_iFilterOrder;
    int exp = ceil(Numerics::log2(iFftLength));
    iFftLength = pow(2, exp);

    if (m_iDesignMethod == 0) {
        double dSmallestFeatureHz = m_dParksWidth * (m_sFreq / 2.0);
        const double dLowEdgeHz = std::max(0.0, (m_dCenterFreq - m_dBandwidth / 2.0) * (m_sFreq / 2.0));
        const double dHighEdgeHz = std::max(0.0, (m_dCenterFreq + m_dBandwidth / 2.0) * (m_sFreq / 2.0));
        const double dSingleCutoffHz = std::max(0.0, m_dCenterFreq * (m_sFreq / 2.0));

        if (m_iFilterType == 0 || m_iFilterType == 1) {
            if (dSingleCutoffHz > 0.0) {
                dSmallestFeatureHz = std::min(dSmallestFeatureHz, dSingleCutoffHz);
            }
        } else {
            if (dLowEdgeHz > 0.0) {
                dSmallestFeatureHz = std::min(dSmallestFeatureHz, dLowEdgeHz);
            }
            if (dHighEdgeHz > 0.0) {
                dSmallestFeatureHz = std::min(dSmallestFeatureHz, dHighEdgeHz);
            }
        }

        dSmallestFeatureHz = std::max(0.5, dSmallestFeatureHz);

        const int iRecommendedFftLength = static_cast<int>(std::ceil((m_sFreq / dSmallestFeatureHz) * 8.0));
        if (iRecommendedFftLength > iFftLength) {
            int iRecommendedExp = ceil(Numerics::log2(iRecommendedFftLength));
            iFftLength = pow(2, iRecommendedExp);
        }
    }

    switch (m_iDesignMethod) {
        case 1: {
            ParksMcClellan filter(m_iFilterOrder,
                                  m_dCenterFreq,
                                  m_dBandwidth,
                                  m_dParksWidth,
                                  static_cast<ParksMcClellan::TPassType>(m_iFilterType));
            m_vecCoeff = filter.coefficients;

            //fft-transform m_vecCoeff in order to be able to perform frequency-domain filtering
            fft_transform_coeffs(iFftLength);

            break;
        }

        case 0: {
            CosineFilter filtercos;

            switch (m_iFilterType) {
                case 0:
                    filtercos = CosineFilter(iFftLength,
                                             (m_dCenterFreq) * (m_sFreq / 2.),
                                             m_dParksWidth * (m_sFreq / 2),
                                             (m_dCenterFreq) * (m_sFreq / 2),
                                             m_dParksWidth * (m_sFreq / 2),
                                             m_sFreq,
                                             static_cast<CosineFilter::TPassType>(m_iFilterType));

                    break;

                case 1:
                    filtercos = CosineFilter(iFftLength,
                                             (m_dCenterFreq) * (m_sFreq / 2),
                                             m_dParksWidth * (m_sFreq / 2),
                                             (m_dCenterFreq) * (m_sFreq / 2),
                                             m_dParksWidth * (m_sFreq / 2),
                                             m_sFreq,
                                             static_cast<CosineFilter::TPassType>(m_iFilterType));

                    break;

                case 2:
                case 3:
                    filtercos = CosineFilter(iFftLength,
                                             (m_dCenterFreq + m_dBandwidth / 2) * (m_sFreq / 2),
                                             m_dParksWidth * (m_sFreq / 2),
                                             (m_dCenterFreq - m_dBandwidth / 2) * (m_sFreq / 2),
                                             m_dParksWidth * (m_sFreq / 2),
                                             m_sFreq,
                                             static_cast<CosineFilter::TPassType>(m_iFilterType));

                    break;
            }

            // Cut the circular zero-phase impulse response to the taps around t = 0: tap m_iFilterOrder / 2 is t = 0
            m_vecCoeff.resize(m_iFilterOrder);
            m_vecCoeff.head(m_iFilterOrder / 2) = filtercos.m_vecCoeff.tail(m_iFilterOrder / 2);
            m_vecCoeff.tail(m_iFilterOrder - m_iFilterOrder / 2) = filtercos.m_vecCoeff.head(m_iFilterOrder - m_iFilterOrder / 2);

            //Now generate the fft version of the shortened impulse response
            fft_transform_coeffs(iFftLength);

            break;
        }
    }

    switch (m_iFilterType) {
        case 0:
            m_dHighpassFreq = 0;
            m_dLowpassFreq = m_dCenterFreq * (m_sFreq / 2);
            break;

        case 1:
            m_dHighpassFreq = m_dCenterFreq * (m_sFreq / 2);
            m_dLowpassFreq = m_sFreq / 2;
            break;

        case 2:
        case 3:
            m_dLowpassFreq = (m_dCenterFreq + m_dBandwidth / 2) * (m_sFreq / 2);
            m_dHighpassFreq = (m_dCenterFreq - m_dBandwidth / 2) * (m_sFreq / 2);
            break;
    }
}

//=============================================================================================================

std::string FilterKernel::short_description() const
{
    return std::format("{}  -  {:.4g}Hz to {:.4g}Hz  -  Ord: {}", design_method().name(), m_dHighpassFreq, m_dLowpassFreq,
                       m_iFilterOrder);
}

//=============================================================================================================

const mne::FilterParameter& FilterKernel::design_method() const
{
    if (m_iDesignMethod < 0) {
        return design_methods.at(0);
    }
    return design_methods.at(m_iDesignMethod);
}

//=============================================================================================================

const mne::FilterParameter& FilterKernel::filter_type() const
{
    if (m_iFilterType < 0) {
        return filter_types.at(0);
    }
    return filter_types.at(m_iFilterType);
}

//=============================================================================================================

void FilterKernel::set_design_method(int iDesignMethod)
{
    m_iDesignMethod = (iDesignMethod >= 0 && iDesignMethod < std::ssize(design_methods)) ? iDesignMethod : 0;
}

//=============================================================================================================

void FilterKernel::set_filter_type(int iFilterType)
{
    m_iFilterType = (iFilterType >= 0 && iFilterType < std::ssize(filter_types)) ? iFilterType : 0;
}

//=============================================================================================================

FilterParameter::FilterParameter()
: FilterParameter("Unknown", "")
{
}

//=============================================================================================================

FilterParameter::FilterParameter(std::string sName)
: FilterParameter(std::move(sName), "")
{
}

//=============================================================================================================

FilterParameter::FilterParameter(std::string sName,
                                 std::string sDescription)
: m_sName(std::move(sName))
, m_sDescription(std::move(sDescription))
{
}

//=============================================================================================================

const std::string& FilterParameter::name() const
{
    return m_sName;
}

//=============================================================================================================

int FilterKernel::design_method_index(std::string_view sName)
{
    return indexOfName(design_methods, sName);
}

//=============================================================================================================

int FilterKernel::filter_type_index(std::string_view sName)
{
    return indexOfName(filter_types, sName);
}
