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
    switch(designMethod) {
        case Tschebyscheff: {
            ParksMcClellan filter(order, centerfreq, bandwidth, parkswidth, (ParksMcClellan::TPassType)type);
            m_dCoeffA = filter.FirCoeff;

            //fft-transform m_dCoeffA in order to be able to perform frequency-domain filtering
            fftTransformCoeffs();

            break;
        }

        case Cosine: {
            m_iFilterOrder = 0;

            CosineFilter filtercos;

            switch(type) {
                case FilterType::LPF:
                    filtercos = CosineFilter (fftlength,
                                            (centerfreq)*(sFreq/2),
                                            parkswidth*(sFreq/2),
                                            (centerfreq)*(sFreq/2),
                                            parkswidth*(sFreq/2),
                                            sFreq,
                                            (CosineFilter::TPassType)type);
                    break;

                case FilterType::HPF:
                    filtercos = CosineFilter (fftlength,
                                            (centerfreq)*(sFreq/2),
                                            parkswidth*(sFreq/2),
                                            (centerfreq)*(sFreq/2),
                                            parkswidth*(sFreq/2),
                                            sFreq,
                                            (CosineFilter::TPassType)type);
                    break;

                case FilterType::BPF:
                    filtercos = CosineFilter (fftlength,
                                            (centerfreq + bandwidth/2)*(sFreq/2),
                                            parkswidth*(sFreq/2),
                                            (centerfreq - bandwidth/2)*(sFreq/2),
                                            parkswidth*(sFreq/2),
                                            sFreq,
                                            (CosineFilter::TPassType)type);
                case FilterType::NOTCH:
                    break;
            }

            m_dCoeffA = filtercos.m_vecCoeff;
            m_dFFTCoeffA = filtercos.m_vecFftCoeff;

            break;
        }
    }
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
    fft.fwd(m_dFFTCoeffA,t_coeffAzeroPad);
}


//*************************************************************************************************************

RowVectorXd FilterOperator::applyFFTFilter(const RowVectorXd& data) const
{
    #ifdef EIGEN_FFTW_DEFAULT
        fftw_make_planner_thread_safe();
    #endif

    //Zero pad in front and back
    RowVectorXd t_dataZeroPad = RowVectorXd::Zero(m_iFFTlength);
    t_dataZeroPad.segment(m_iFFTlength/4-m_iFilterOrder/2, data.cols()) = data;

    //generate fft object
    Eigen::FFT<double> fft;
    fft.SetFlag(fft.HalfSpectrum);

    //fft-transform data sequence
    RowVectorXcd t_freqData;
    fft.fwd(t_freqData,t_dataZeroPad);

    //perform frequency-domain filtering
    RowVectorXcd t_filteredFreq = m_dFFTCoeffA.array()*t_freqData.array();

    //inverse-FFT
    RowVectorXd t_filteredTime;
    fft.inv(t_filteredTime,t_filteredFreq);

    //Return filtered data still with zeros at front and end
    return t_filteredTime;
}
