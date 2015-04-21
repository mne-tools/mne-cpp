//=============================================================================================================
/**
* @file     filterdata.cpp
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


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;


//*************************************************************************************************************

FilterData::FilterData()
{

}


//*************************************************************************************************************

FilterData::FilterData(QString unique_name, FilterType type, int order, double centerfreq, double bandwidth, double parkswidth, double sFreq, qint32 fftlength, DesignMethod designMethod)
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
    //std::cout<<"START FilterData::FilterData()"<<std::endl;
    designFilter();
    //std::cout<<"END FilterData::FilterData()"<<std::endl;
}


//*************************************************************************************************************

FilterData::FilterData(QString &path)
{
    //std::cout<<"START FilterData::FilterData()"<<std::endl;

    QString type;

    if(LoadFilter::readFilterFile(path, m_dCoeffA, type, m_sName, m_iFilterOrder)) {
        if(type == "HPF")
            m_Type = FilterData::HPF;

        if(type == "BPF")
            m_Type = FilterData::BPF;

        if(type == "LPF")
            m_Type = FilterData::LPF;

        fftTransformCoeffs();
    }
    else
        qDebug()<<"Could not read filter file!";

    //std::cout<<"END FilterData::FilterData()"<<std::endl;
}


//*************************************************************************************************************

void FilterData::designFilter()
{
    switch(m_designMethod) {
        case Tschebyscheff: {
            ParksMcClellan filter(m_iFilterOrder, m_dCenterFreq, m_dBandwidth, m_dParksWidth, (ParksMcClellan::TPassType)m_Type);
            m_dCoeffA = filter.FirCoeff;

            //fft-transform m_dCoeffA in order to be able to perform frequency-domain filtering
            fftTransformCoeffs();

            break;
        }

        case Cosine: {
            CosineFilter filtercos;

            switch(m_Type) {
                case LPF:
                    filtercos = CosineFilter (m_iFFTlength,
                                            (m_dCenterFreq)*(m_sFreq/2),
                                            m_dParksWidth*(m_sFreq/2),
                                            (m_dCenterFreq)*(m_sFreq/2),
                                            m_dParksWidth*(m_sFreq/2),
                                            m_sFreq,
                                            (CosineFilter::TPassType)m_Type);
                    break;

                case HPF:
                    filtercos = CosineFilter (m_iFFTlength,
                                            (m_dCenterFreq)*(m_sFreq/2),
                                            m_dParksWidth*(m_sFreq/2),
                                            (m_dCenterFreq)*(m_sFreq/2),
                                            m_dParksWidth*(m_sFreq/2),
                                            m_sFreq,
                                            (CosineFilter::TPassType)m_Type);
                    break;

                case BPF:
                    filtercos = CosineFilter (m_iFFTlength,
                                            (m_dCenterFreq + m_dBandwidth/2)*(m_sFreq/2),
                                            m_dParksWidth*(m_sFreq/2),
                                            (m_dCenterFreq - m_dBandwidth/2)*(m_sFreq/2),
                                            m_dParksWidth*(m_sFreq/2),
                                            m_sFreq,
                                            (CosineFilter::TPassType)m_Type);
                    break;
            }

            m_dCoeffA = filtercos.m_dCoeffA.head(m_iFilterOrder);
            m_dFFTCoeffA = filtercos.m_dFFTCoeffA;

            break;
        }
    }

}

//*************************************************************************************************************

void FilterData::fftTransformCoeffs()
{
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

RowVectorXd FilterData::applyConvFilter(const RowVectorXd& data) const
{
    //Zero pad in front and make filter coeff causal
    RowVectorXd dCoeffA = m_dCoeffA.head(m_dCoeffA.cols()/2).reverse();

    std::cout<<"dCoeffA.cols(): "<<dCoeffA.cols()<<std::endl;
    //dCoeffA(0) = 0;

    RowVectorXd t_dataZeroPad = RowVectorXd::Zero(2*dCoeffA.cols() + data.cols());
    t_dataZeroPad.segment(dCoeffA.cols(), data.cols()) = data;

    RowVectorXd t_filteredTime = RowVectorXd::Zero(data.cols()+2*dCoeffA.cols());

    for(int i=dCoeffA.cols(); i<dCoeffA.cols()+data.cols(); i++) {
        for(int u=0; u<dCoeffA.cols(); u++) {
            t_filteredTime(i-dCoeffA.cols()) += t_dataZeroPad(i-u) * dCoeffA(u);
        }
    }

    if(m_designMethod == Cosine)
        return t_filteredTime.segment(dCoeffA.cols(),data.cols());

    return t_filteredTime.head(data.cols());
}


//*************************************************************************************************************

RowVectorXd FilterData::applyFFTFilter(const RowVectorXd& data, bool keepOverhead, CompensateEdgeEffects compensateEdgeEffects) const
{
    //Zero pad or mirror in front and back of the data
    RowVectorXd t_dataZeroPad = RowVectorXd::Zero(m_iFFTlength);

//    std::cout<<"data.cols()"<<data.cols()<<std::endl;
//    std::cout<<"m_iFFTlength"<<m_iFFTlength<<std::endl;

    //Calculate starting and end point of data block
    int startData = m_iFFTlength/2-data.cols()/2;
    int dataInFFTLength = m_iFFTlength/data.cols();
    int dataInFFTLengthRest = m_iFFTlength%data.cols();

    if(dataInFFTLengthRest!=0)
        dataInFFTLength++;

    switch(compensateEdgeEffects) {
        case MirrorData:
            //Fill front and back with available data as much as possible
            for(int i=0; i<dataInFFTLength; i++)
                if(dataInFFTLengthRest!=0 && i==dataInFFTLength-1)
                    if(i%2==0)
                        t_dataZeroPad.segment(i*data.cols(),dataInFFTLengthRest) = data.head(dataInFFTLengthRest).reverse();
                    else
                        t_dataZeroPad.segment(i*data.cols(),dataInFFTLengthRest) = data.head(dataInFFTLengthRest);
                else
                    if(i%2==0)
                        t_dataZeroPad.segment(i*data.cols(),data.cols()) = data.reverse();
                    else
                        t_dataZeroPad.segment(i*data.cols(),data.cols()) = data;

            t_dataZeroPad.segment(startData, data.cols()) = data; //actual data

//            startData = data.cols();
//            t_dataZeroPad.head(data.cols()) = data.reverse(); //front
//            t_dataZeroPad.segment(data.cols(), data.cols()) = data; //middle
//            t_dataZeroPad.segment(2*data.cols(), data.cols()) = data.reverse(); //back
            break;

        case ZeroPad:
            t_dataZeroPad.segment(m_iFilterOrder/2, data.cols()) = data;
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

    //Return filtered data still with zeros at front and end depending on keepZeros flag
    if(!keepOverhead) {
        switch(compensateEdgeEffects) {
            case MirrorData:
                switch(m_designMethod) {
                    case Cosine:
                        return t_filteredTime.segment(startData, data.cols());

                    case Tschebyscheff:
                        return t_filteredTime.segment(startData+m_iFilterOrder/2, data.cols());
                }
                break;

            case ZeroPad:
                switch(m_designMethod) {
                    case Cosine:
                        return t_filteredTime.segment(0, data.cols());

                    case Tschebyscheff:
                        return t_filteredTime.segment(m_iFilterOrder/2, data.cols());
                }
                break;
        }
    }

    return t_filteredTime;
}


//OLD
////*************************************************************************************************************

//RowVectorXd FilterData::applyFFTFilter(const RowVectorXd& data, bool keepZeros) const
//{
//    //Zero pad in front and back
//    RowVectorXd t_dataZeroPad = RowVectorXd::Zero(m_iFFTlength);
//    t_dataZeroPad.segment(m_iFFTlength/4-m_iFilterOrder/2, data.cols()) = data;

//    //generate fft object
//    Eigen::FFT<double> fft;
//    fft.SetFlag(fft.HalfSpectrum);

//    //fft-transform data sequence
//    RowVectorXcd t_freqData;
//    fft.fwd(t_freqData,t_dataZeroPad);

//    //perform frequency-domain filtering
//    RowVectorXcd t_filteredFreq = m_dFFTCoeffA.array()*t_freqData.array();

//    //inverse-FFT
//    RowVectorXd t_filteredTime;
//    fft.inv(t_filteredTime,t_filteredFreq);

//    //Return filtered data still with zeros at front and end depending on keepZeros flag
//    if(!keepZeros)
//        if(m_designMethod == Tschebyscheff)
//            return t_filteredTime.segment(m_iFFTlength/4, data.cols());
//        else
//            return t_filteredTime.segment(m_iFFTlength/4-m_iFilterOrder/2, data.cols());

//    return t_filteredTime;
//}
