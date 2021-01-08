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
#include <QVector>

//=============================================================================================================
// DEFINE NAMESPACE RTPROCESSINGLIB
//=============================================================================================================

namespace RTPROCESSINGLIB
{

//=============================================================================================================
/**
 * @brief The FilterParameter class
 */
class RTPROCESINGSHARED_EXPORT FilterParameter{
friend class FilterKernel;

public:
    explicit FilterParameter(QString);
    explicit FilterParameter(QString, QString);

    QString getName() const;

    friend bool operator == (const FilterParameter& in1, const FilterParameter& in2){
        return (in1.m_sName == in2.m_sName);
    }
protected:
    QString m_sName;            /**< Item name */
    QString m_sDescription;     /**< Item description */
};

//=============================================================================================================
/**
 * The FilterKernel class provides methods to create/design a FIR filter kernel
 *
 * @brief The FilterKernel class provides methods to create/design a FIR filter kernel
 */
class RTPROCESINGSHARED_EXPORT FilterKernel
{

public:
//    enum DesignMethod {
//        Cosine,
//        Tschebyscheff,
//        External
//    } m_designMethod;

//    enum FilterType {
//        LPF,
//        HPF,
//        BPF,
//        NOTCH,
//        UNKNOWN
//    } m_Type;

    //=========================================================================================================
    /**
     * @brief FilterKernel creates a default FilterKernel object
     */
    FilterKernel();

    //=========================================================================================================
    /**
     * Constructs a FilterKernel object
     *
     * @param [in] sFilterName      Defines the name of the generated filter
     * @param [in] type             Tyep of the filter: LPF, HPF, BPF, NOTCH (from enum FilterType)
     * @param [in] iOrder           Represents the order of the filter, the higher the higher is the stopband attenuation
     * @param [in] dCenterfreq      Determines the center of the frequency - normed to sFreq/2 (nyquist)
     * @param [in] dBandwidth       Ignored if FilterType is set to LPF,HPF. if NOTCH/BPF: bandwidth of stop-/passband - normed to sFreq/2 (nyquist)
     * @param [in] dParkswidth      Determines the width of the filter slopes (steepness) - normed to sFreq/2 (nyquist)
     * @param [in] dSFreq           The sampling frequency
     * @param [in] designMethod     Specifies the design method to use. Choose between Cosind and Tschebyscheff
     **/
    FilterKernel(const QString &sFilterName,
                 int  iFilterType,
                 int iOrder,
                 double dCenterfreq,
                 double dBandwidth,
                 double dParkswidth,
                 double dSFreq,
                 int iDesignMethod);

    //=========================================================================================================
    /**
     * Prepares a filter kernel to be used wiht a specific data block length.
     * This is favorable to call before filtering, in order to avoid transforming the
     * filter coefficients anew during filtering. This functions was introduced since
     * one does not always know the data length of the data blocks to be filtered when
     * designing the filter.
     *
     * @param [in] iDataSize           The data size to setup the filters to
     */
    void prepareFilter(int iDataSize);

    //=========================================================================================================
    /**
     * Applies the current filter to the input data using convolution in time domain.
     *
     * @param [in] vecData          Holds the data to be filtered
     * @param [in] bKeepOverhead    Whether the result should still include the overhead information in front and back of the data.
     *                              Default is set to false.
     *
     * @return the filtered data in form of a RowVectorXd
     */
    Eigen::RowVectorXd applyConvFilter(const Eigen::RowVectorXd& vecData,
                                       bool bKeepOverhead = false) const;

    //=========================================================================================================
    /**
     * Applies the current filter to the input data using multiplication in frequency domain.
     *
     * @param [in/out] vecData              Holds the data to be filtered. Gets overwritten with its filtered result.
     * @param [in] bKeepOverhead            Whether the result should still include the overhead information in front and back of the data.
     *                                      Default is set to false.
     *
     * @return the filtered data in form of a RowVectorXd
     */
    void applyFftFilter(Eigen::RowVectorXd& vecData,
                        bool bKeepOverhead = false);

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

    Eigen::RowVectorXd getCoefficients() const;
    void setCoefficients(const Eigen::RowVectorXd& vecCoeff);

    Eigen::RowVectorXcd getFftCoefficients() const;
    void setFftCoefficients(const Eigen::RowVectorXcd& vecFftCoeff);

    inline static const QVector<RTPROCESSINGLIB::FilterParameter> initDesignMethods();
    inline static const QVector<RTPROCESSINGLIB::FilterParameter> initFilterTypes();

    static const QVector<RTPROCESSINGLIB::FilterParameter>  m_designMethods;  /**< Vector of possible filter design methods */
    static const QVector<RTPROCESSINGLIB::FilterParameter>  m_filterTypes;    /**< Vector of possible filter design types */

    RTPROCESSINGLIB::FilterParameter getDesignMethod() const;
    RTPROCESSINGLIB::FilterParameter getFilterType() const;

private:
    //=========================================================================================================
    /**
     * Transforms the calculated filter coefficients to frequency-domain
     *
     * @param [in] iFftLength                  The FFT length to use
     */
    bool fftTransformCoeffs(int iFftLength);

    //=========================================================================================================
    /**
     * Designs the actual filter with the given parameters
     */
    void designFilter();

    double          m_sFreq;                /**< the sampling frequency. */
    double          m_dCenterFreq;          /**< contains center freq of the filter. */
    double          m_dBandwidth;           /**< contains bandwidth of the filter. */
    double          m_dParksWidth;          /**< contains the parksmcallen width. */
    double          m_dLowpassFreq;         /**< lowpass freq (higher cut off) of the filter. */
    double          m_dHighpassFreq;        /**< lowpass freq (lower cut off) of the filter. */

    int             m_iFilterOrder;         /**< represents the order of the filter instance. */

    QString         m_sFilterName;          /**< contains name of the filter. */

    Eigen::RowVectorXd     m_vecCoeff;       /**< contains the forward filter coefficient set. */
    Eigen::RowVectorXcd    m_vecFftCoeff;    /**< the FFT-transformed forward filter coefficient set, required for frequency-domain filtering, zero-padded to m_iFftLength. */

    int m_iDesignMethod;
    int m_iFilterType;
};

const QVector<RTPROCESSINGLIB::FilterParameter> FilterKernel::m_designMethods = FilterKernel::initDesignMethods();
const QVector<RTPROCESSINGLIB::FilterParameter> FilterKernel::m_filterTypes = FilterKernel::initFilterTypes();


//=========================================================================================================
/**
 * Returns the current design method as a string
 */
RTPROCESINGSHARED_EXPORT QString getStringForDesignMethod(FilterKernel::DesignMethod designMethod);

//=========================================================================================================
/**
 * Returns the current filter type as a string
 */
RTPROCESINGSHARED_EXPORT QString getStringForFilterType(FilterKernel::FilterType filterType);

//=========================================================================================================
/**
 * Returns the current design dependent on an input string
 */
RTPROCESINGSHARED_EXPORT FilterKernel::DesignMethod getDesignMethodForString(const QString& designMethodString);

//=========================================================================================================
/**
 * Returns the current filter type dependent on an input string
 */
RTPROCESINGSHARED_EXPORT FilterKernel::FilterType getFilterTypeForString(const QString& filerTypeString);

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline const QVector<RTPROCESSINGLIB::FilterParameter> FilterKernel::initDesignMethods()
{
    QVector<RTPROCESSINGLIB::FilterParameter> designMethods;
    designMethods.append(FilterParameter(QString("Cosine"), QString("A cosine filter")));
    designMethods.append(FilterParameter(QString("Tschebyscheff"), QString("A tschebyscheff filter")));
    designMethods.append(FilterParameter(QString("External"), QString("An external filter")));

    return designMethods;
}

//=========================================================================================================

inline const QVector<RTPROCESSINGLIB::FilterParameter> FilterKernel::initFilterTypes()
{
    QVector<RTPROCESSINGLIB::FilterParameter> filterTypes;
    filterTypes.append(FilterParameter(QString("LPF"), QString("An LPF filter")));
    filterTypes.append(FilterParameter(QString("HPF"), QString("An HPF filter")));
    filterTypes.append(FilterParameter(QString("BPF"), QString("A BPF filter")));
    filterTypes.append(FilterParameter(QString("NOTCH"), QString("A NOTCH filter")));
    filterTypes.append(FilterParameter(QString("UNKNOWN"), QString("An UNKNOWN filter")));

    return filterTypes;
}

} // NAMESPACE RTPROCESSINGLIB

//#ifndef metatype_filtertype
//#define metatype_filtertype
//Q_DECLARE_METATYPE(RTPROCESSINGLIB::FilterKernel::FilterType)
//#endif

//#ifndef metatype_filterdesign
//#define metatype_filterdesign
//Q_DECLARE_METATYPE(RTPROCESSINGLIB::FilterKernel::DesignMethod)
//#endif

#ifndef metatype_filterkernel
#define metatype_filterkernel
Q_DECLARE_METATYPE(RTPROCESSINGLIB::FilterKernel)
#endif

#endif // FILTERKERNEL_H
