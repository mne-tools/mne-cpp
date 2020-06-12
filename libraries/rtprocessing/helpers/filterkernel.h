//=============================================================================================================
/**
 * @file     filterkernel.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Ruben Doerfel <Ruben.Doerfel@tu-ilmenau.de>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.3
 * @date     June, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch, Ruben Doerfel, Christoph Dinh. All rights reserved.
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
 * @brief    The FilterKernel class represents a filter object that generates the FIR filter coefficients using Park's McClellan's
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

#ifndef FILTERKERNEL_H
#define FILTERKERNEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../rtprocessing_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QMetaType>

//=============================================================================================================
// DEFINE NAMESPACE RTPROCESSINGLIB
//=============================================================================================================

namespace RTPROCESSINGLIB
{

//=============================================================================================================
/**
 * DECLARE CLASS FilterSettingsView
 *
 * @brief The FilterSettingsView class provides a view to select between different modalities
 */
class RTPROCESINGSHARED_EXPORT FilterKernel
{

public:
    enum DesignMethod {
        Tschebyscheff,
        Cosine,
        External
    } m_designMethod;

    enum FilterType {
        LPF,
        HPF,
        BPF,
        NOTCH,
        UNKNOWN
    } m_Type;

    //=========================================================================================================
    /**
     * @brief FilterKernel creates a default FilterKernel object
     */
    FilterKernel();

    //=========================================================================================================
    /**
     * Constructs a FilterKernel object
     *
     * @param [in] sFilterName      defines the name of the generated filter
     * @param [in] type             of the filter: LPF, HPF, BPF, NOTCH (from enum FilterType)
     * @param [in] iOrder            represents the order of the filter, the higher the higher is the stopband attenuation
     * @param [in] dCenterfreq      determines the center of the frequency - normed to sFreq/2 (nyquist)
     * @param [in] dBandwidth       ignored if FilterType is set to LPF,HPF. if NOTCH/BPF: bandwidth of stop-/passband - normed to sFreq/2 (nyquist)
     * @param [in] dParkswidth      determines the width of the filter slopes (steepness) - normed to sFreq/2 (nyquist)
     * @param [in] dSFreq           sampling frequency
     * @param [in] iFftLength       length of the fft (multiple integer of 2^x)
     * @param [in] designMethod     specifies the design method to use. Choose between Cosind and Tschebyscheff
     **/
    FilterKernel(const QString &sFilterName,
                 FilterType type,
                 int iOrder,
                 double dCenterfreq,
                 double dBandwidth,
                 double dParkswidth,
                 double dSFreq,
                 qint32 iFftLength = 4096,
                 DesignMethod designMethod = Cosine);

    //=========================================================================================================
    /**
     * Applies the current filter to the input data using convolution in time domain.
     *
     * @param [in] vecData          holds the data to be filtered
     * @param [in] bKeepOverhead     whether the result should still include the overhead information in front and back of the data
     *
     * @return the filtered data in form of a RoVecotrXd
     */
    Eigen::RowVectorXd applyConvFilter(const Eigen::RowVectorXd& vecData,
                                       bool bKeepOverhead = false) const;

    //=========================================================================================================
    /**
     * Applies the current filter to the input data using multiplication in frequency domain.
     *
     * @param [in] vecData                  holds the data to be filtered
     * @param [in] bKeepOverhead             whether the result should still include the overhead information in front and back of the data
     *
     * @return the filtered data in form of a RoVecotrXd
     */
    Eigen::RowVectorXd applyFftFilter(const Eigen::RowVectorXd& vecData,
                                      bool bKeepOverhead = false) const;

    //=========================================================================================================
    /**
     * Returns the current design method as a string
     */
    static QString getStringForDesignMethod(const FilterKernel::DesignMethod& designMethod);

    //=========================================================================================================
    /**
     * Returns the current filter type as a string
     */
    static QString getStringForFilterType(const FilterKernel::FilterType& filterType);

    //=========================================================================================================
    /**
     * Returns the current design dependent on an input string
     */
    static FilterKernel::DesignMethod getDesignMethodForString(const QString& designMethodString);

    //=========================================================================================================
    /**
     * Returns the current filter type dependent on an input string
     */
    static FilterKernel::FilterType getFilterTypeForString(const QString& filerTypeString);

    QString getName() const;
    void setName(const QString& sFilterName);

    double getSamplingFrequency() const;
    void setSamplingFrequency(double dSFreq);

    int getFilterOrder() const;
    void setFilterOrder(int iOrder);

    double getCenterFrequency() const;
    void setCenterFrequency(double dCenterFreq);

    double getBandwidth() const;
    void setBandwidth(double dBandwidth);

    double getParksWidth() const;
    void setParksWidth(double dParksWidth);

    double getHighpassFreq() const;
    void setHighpassFreq(double dHighpassFreq);

    double getLowpassFreq() const;
    void setLowpassFreq(double dLowpassFreq);

    int getFftLength() const;
    void setFftLength(int dFftLength);

    Eigen::RowVectorXd getCoefficients() const;
    void setCoefficients(const Eigen::RowVectorXd& vecCoeff);

    Eigen::RowVectorXcd getFftCoefficients() const;
    void setFftCoefficients(const Eigen::RowVectorXcd& vecFftCoeff);

private:
    //=========================================================================================================
    /**
     * @brief fftTransformCoeffs transforms the calculated filter coefficients to frequency-domain
     */
    void fftTransformCoeffs();

    //=========================================================================================================
    /**
     * @brief designFilter designs the actual filter with the given parameters
     */
    void designFilter();

    double          m_sFreq;                /**< the sampling frequency. */
    double          m_dCenterFreq;          /**< contains center freq of the filter. */
    double          m_dBandwidth;           /**< contains bandwidth of the filter. */
    double          m_dParksWidth;          /**< contains the parksmcallen width. */
    double          m_dLowpassFreq;         /**< lowpass freq (higher cut off) of the filter. */
    double          m_dHighpassFreq;        /**< lowpass freq (lower cut off) of the filter. */

    int             m_iFilterOrder;         /**< represents the order of the filter instance. */
    int             m_iFftLength;           /**< represents the filter length. */

    QString         m_sFilterName;          /**< contains name of the filter. */

    Eigen::RowVectorXd     m_vecCoeff;       /**< contains the forward filter coefficient set. */
    Eigen::RowVectorXcd    m_vecFftCoeff;    /**< the FFT-transformed forward filter coefficient set, required for frequency-domain filtering, zero-padded to m_iFftLength. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE RTPROCESSINGLIB

#ifndef metatype_filtertype
#define metatype_filtertype
Q_DECLARE_METATYPE(RTPROCESSINGLIB::FilterKernel::FilterType)
#endif

#ifndef metatype_filterdesign
#define metatype_filterdesign
Q_DECLARE_METATYPE(RTPROCESSINGLIB::FilterKernel::DesignMethod)
#endif

#ifndef metatype_filterdata
#define metatype_filterdata
Q_DECLARE_METATYPE(RTPROCESSINGLIB::FilterKernel)
#endif

#endif // FILTERKERNEL_H
