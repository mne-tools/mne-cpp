//=============================================================================================================
/**
 * @file     filterdata.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Ruben Doerfel <Ruben.Doerfel@tu-ilmenau.de>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  dev
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
 * @brief    Contains all FilterData.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filterdata.h"

#include "../mnemath.h"

#include "parksmcclellan.h"
#include "cosinefilter.h"

#include <iostream>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//*************************************************************************************************************
//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/SparseCore>
//#ifndef EIGEN_FFTW_DEFAULT
//#define EIGEN_FFTW_DEFAULT
//#endif
#include <unsupported/Eigen/FFT>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;

//*************************************************************************************************************

FilterData::FilterData()
: m_Type(UNKNOWN)
, m_iFilterOrder(80)
, m_iFFTlength(512)
, m_sName("Unknown")
, m_dParksWidth(0.1)
, m_designMethod(External)
, m_dCenterFreq(0.5)
, m_dBandwidth(0.1)
, m_sFreq(1000)
, m_dLowpassFreq(4)
, m_dHighpassFreq(40)
{

}


//*************************************************************************************************************

FilterData::FilterData(QString unique_name,
                       FilterType type,
                       int order,
                       double centerfreq,
                       double bandwidth,
                       double parkswidth,
                       double sFreq,
                       qint32 fftlength,
                       DesignMethod designMethod)
: m_Type(type)
, m_iFilterOrder(order)
, m_iFFTlength(fftlength)
, m_sName(unique_name)
, m_dParksWidth(parkswidth)
, m_designMethod(designMethod)
, m_dCenterFreq(centerfreq)
, m_dBandwidth(bandwidth)
, m_sFreq(sFreq)
{
    designFilter();
}


//*************************************************************************************************************

void FilterData::designFilter()
{
    switch(m_designMethod) {
        case Tschebyscheff: {
            ParksMcClellan filter(m_iFilterOrder,
                                  m_dCenterFreq,
                                  m_dBandwidth,
                                  m_dParksWidth,
                                  (ParksMcClellan::TPassType)m_Type);
            m_dCoeffA = filter.FirCoeff;

            //fft-transform m_dCoeffA in order to be able to perform frequency-domain filtering
            fftTransformCoeffs();

            break;
        }

        case Cosine: {
            CosineFilter filtercos;

            switch(m_Type) {
                case LPF:
                    filtercos = CosineFilter(m_iFFTlength,
                                             (m_dCenterFreq)*(m_sFreq/2),
                                             m_dParksWidth*(m_sFreq/2),
                                             (m_dCenterFreq)*(m_sFreq/2),
                                             m_dParksWidth*(m_sFreq/2),
                                             m_sFreq,
                                             (CosineFilter::TPassType)m_Type);

                    break;

                case HPF:
                    filtercos = CosineFilter(m_iFFTlength,
                                             (m_dCenterFreq)*(m_sFreq/2),
                                             m_dParksWidth*(m_sFreq/2),
                                             (m_dCenterFreq)*(m_sFreq/2),
                                             m_dParksWidth*(m_sFreq/2),
                                             m_sFreq,
                                             (CosineFilter::TPassType)m_Type);

                    break;

                case BPF:
                    filtercos = CosineFilter(m_iFFTlength,
                                             (m_dCenterFreq + m_dBandwidth/2)*(m_sFreq/2),
                                             m_dParksWidth*(m_sFreq/2),
                                             (m_dCenterFreq - m_dBandwidth/2)*(m_sFreq/2),
                                             m_dParksWidth*(m_sFreq/2),
                                             m_sFreq,
                                             (CosineFilter::TPassType)m_Type);

                    break;
            }

            //This filter is designed in the frequency domain, hence the time domain impulse response need to be shortend by the users dependent number of taps
            m_dCoeffA.resize(m_iFilterOrder);
            m_dCoeffA.head(m_iFilterOrder/2) = filtercos.m_dCoeffA.tail(m_iFilterOrder/2);
            m_dCoeffA.tail(m_iFilterOrder/2) = filtercos.m_dCoeffA.head(m_iFilterOrder/2);

            //Now generate the fft version of the shortened impulse response
            fftTransformCoeffs();

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


//*************************************************************************************************************

void FilterData::fftTransformCoeffs()
{
    #ifdef EIGEN_FFTW_DEFAULT
        fftw_make_planner_thread_safe();
    #endif

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

RowVectorXd FilterData::applyConvFilter(const RowVectorXd& data, bool keepOverhead, CompensateEdgeEffects compensateEdgeEffects) const
{
    if(data.cols()<m_dCoeffA.cols() && compensateEdgeEffects==MirrorData){
        qDebug()<<QString("Error in FilterData: Number of filter taps(%1) bigger then data size(%2). Not enough data to perform mirroring!").arg(m_dCoeffA.cols()).arg(data.cols());
        return data;
    }

    //Do zero padding or mirroring depending on user input
    RowVectorXd t_dataZeroPad = RowVectorXd::Zero(2*m_dCoeffA.cols() + data.cols());
    RowVectorXd t_filteredTime = RowVectorXd::Zero(2*m_dCoeffA.cols() + data.cols());

    switch(compensateEdgeEffects) {
        case MirrorData:
            t_dataZeroPad.head(m_dCoeffA.cols()) = data.head(m_dCoeffA.cols()).reverse();   //front
            t_dataZeroPad.segment(m_dCoeffA.cols(), data.cols()) = data;                    //middle
            t_dataZeroPad.tail(m_dCoeffA.cols()) = data.tail(m_dCoeffA.cols()).reverse();   //back
            break;

        case ZeroPad:
            t_dataZeroPad.segment(m_dCoeffA.cols(), data.cols()) = data;
            break;

        default:
            t_dataZeroPad.segment(m_dCoeffA.cols(), data.cols()) = data;
            break;
    }

    //Do the convolution
    for(int i=m_dCoeffA.cols(); i<t_filteredTime.cols(); i++)
        t_filteredTime(i-m_dCoeffA.cols()) = t_dataZeroPad.segment(i-m_dCoeffA.cols(),m_dCoeffA.cols()) * m_dCoeffA.transpose();

    //Return filtered data
    if(!keepOverhead)
        return t_filteredTime.segment(m_dCoeffA.cols()/2, data.cols());

    return t_filteredTime.head(data.cols()+m_dCoeffA.cols());
}


//*************************************************************************************************************

RowVectorXd FilterData::applyFFTFilter(const RowVectorXd& data, bool keepOverhead, CompensateEdgeEffects compensateEdgeEffects) const
{
    #ifdef EIGEN_FFTW_DEFAULT
        fftw_make_planner_thread_safe();
    #endif

    if(data.cols()<m_dCoeffA.cols()/2 && compensateEdgeEffects==MirrorData) {
        qDebug()<<QString("Error in FilterData: Number of filter taps(%1) bigger then data size(%2). Not enough data to perform mirroring!").arg(m_dCoeffA.cols()).arg(data.cols());
        return data;
    }

    if(m_dCoeffA.cols() + data.cols()>m_iFFTlength) {
        qDebug()<<"Error in FilterData: Number of mirroring/zeropadding size plus data size is bigger then fft length!";
        return data;
    }

    //Do zero padding or mirroring depending on user input
    RowVectorXd t_dataZeroPad = RowVectorXd::Zero(m_iFFTlength);

    switch(compensateEdgeEffects) {
        case MirrorData:
            t_dataZeroPad.head(m_dCoeffA.cols()/2) = data.head(m_dCoeffA.cols()/2).reverse();   //front
            t_dataZeroPad.segment(m_dCoeffA.cols()/2, data.cols()) = data;                    //middle
            t_dataZeroPad.tail(m_dCoeffA.cols()/2) = data.tail(m_dCoeffA.cols()/2).reverse();   //back
            break;

        case ZeroPad:
            t_dataZeroPad.head(data.cols()) = data;
            break;

        default:
            t_dataZeroPad.head(data.cols()) = data;
            break;
    }

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

    //Return filtered data
    if(!keepOverhead)
        return t_filteredTime.segment(m_dCoeffA.cols()/2, data.cols());

    return t_filteredTime.head(data.cols()+m_dCoeffA.cols());
}


//*************************************************************************************************************

QString FilterData::getStringForDesignMethod(const FilterData::DesignMethod &designMethod)
{
    QString designMethodString = "External";

    if(designMethod == FilterData::External)
        designMethodString = "External";

    if(designMethod == FilterData::Cosine)
        designMethodString = "Cosine";

    if(designMethod == FilterData::Tschebyscheff)
        designMethodString = "Tschebyscheff";

    return designMethodString;
}


//*************************************************************************************************************

QString FilterData::getStringForFilterType(const FilterData::FilterType &filterType)
{
    QString filterTypeString = "LPF";

    if(filterType == FilterData::LPF)
        filterTypeString = "LPF";

    if(filterType == FilterData::HPF)
        filterTypeString = "HPF";

    if(filterType == FilterData::BPF)
        filterTypeString = "BPF";

    if(filterType == FilterData::NOTCH)
        filterTypeString = "NOTCH";

    return filterTypeString;
}


//*************************************************************************************************************

FilterData::DesignMethod FilterData::getDesignMethodForString(const QString &designMethodString)
{
    FilterData::DesignMethod designMethod = FilterData::External;

    if(designMethodString == "External")
        designMethod = FilterData::External;

    if(designMethodString == "Tschebyscheff")
        designMethod = FilterData::Tschebyscheff;

    if(designMethodString == "Cosine")
        designMethod = FilterData::Cosine;

    return designMethod;
}


//*************************************************************************************************************

FilterData::FilterType FilterData::getFilterTypeForString(const QString &filterTypeString)
{
    FilterData::FilterType filterType = FilterData::UNKNOWN;

    if(filterTypeString == "LPF")
        filterType = FilterData::LPF;

    if(filterTypeString == "HPF")
        filterType = FilterData::HPF;

    if(filterTypeString == "BPF")
        filterType = FilterData::BPF;

    if(filterTypeString == "NOTCH")
        filterType = FilterData::NOTCH;

    return filterType;
}
