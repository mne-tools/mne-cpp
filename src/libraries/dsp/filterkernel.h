//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file filterkernel.h
 * @since March 2026
 * @brief Linear-phase FIR filter kernel with overlap-add FFT convolution back-end.
 *
 * FilterKernel is the core FIR engine of the DSP library: it owns the impulse
 * response coefficients of a low-pass, high-pass, band-pass or notch filter
 * together with their zero-padded FFT and applies them to incoming data via
 * overlap-add convolution. Two design back-ends are supported — the
 * cosine-tapered raised-cosine design (@ref CosineFilter, fast, smooth
 * roll-off) and the Parks–McClellan equiripple design
 * (@ref ParksMcClellan, optimal minimax behaviour). Both produce Type-I
 * linear-phase responses, so the kernel can be applied either as a single
 * forward pass with a fixed group delay of @c (NumTaps-1)/2 samples or in
 * zero-phase forward / time-reverse mode.
 *
 * Frequency-domain convolution is realised by zero-padding the impulse
 * response to the next power of two, transforming once at design time and
 * caching the result. Each input block of length @c FFTLength − @c NumTaps
 * is then transformed, multiplied with the cached spectrum and inverse-
 * transformed; consecutive blocks are stitched by adding the @c NumTaps − 1
 * tail of the preceding block to the head of the next, which suppresses
 * circular-convolution artefacts. Typical example: with @c FFTLength = 4096
 * and @c NumTaps = 80 the useful per-block input length is 4016 samples.
 *
 * References:
 *   [1] https://en.wikipedia.org/wiki/Parks%E2%80%93McClellan_filter_design_algorithm
 *   [2] https://en.wikipedia.org/wiki/Overlap%E2%80%93add_method
 */

#ifndef FILTERKERNEL_H
#define FILTERKERNEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"
//#include "filter.h"

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
#include <QDebug>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * @brief Named filter-design parameter descriptor holding a human-readable name and description (e.g. design method or filter type).
 */
class DSPSHARED_EXPORT FilterParameter{

public:
    //=========================================================================================================
    /**
     * Creates a filter parameter with name 'Unknown'
     */
    explicit FilterParameter();

    //=========================================================================================================
    /**
     * Creates a filter parameter with name sName
     *
     * @param[in] sName    input name.
     */
    explicit FilterParameter(QString sName);

    //=========================================================================================================
    /**
     * Creates a filter parameter with name sName and description sDescription
     *
     * @param[in] sName            input name.
     * @param[in] sDescription     input desxription.
     */
    explicit FilterParameter(QString sName, QString sDescription);

    //=========================================================================================================
    /**
     * Returns parameter name
     *
     * @return Parameter name.
     */
    QString getName() const;

    friend bool operator == (const FilterParameter& in1, const FilterParameter& in2){
        //qDebug() << in1.getName() << in2.getName();
        return (in1.getName() == in2.getName());
    }
protected:
    QString m_sName;            /**< Item name. */
    QString m_sDescription;     /**< Item description. */
};

//=============================================================================================================
/**
 * The FilterKernel class provides methods to create/design a FIR filter kernel
 *
 * @brief The FilterKernel class provides methods to create/design a FIR filter kernel
 */
class DSPSHARED_EXPORT FilterKernel
{

public:
    //=========================================================================================================
    /**
     * @brief FilterKernel creates a default FilterKernel object
     */
    FilterKernel();

    //=========================================================================================================
    /**
     * Constructs a FilterKernel object
     *
     * @param[in] sFilterName      Defines the name of the generated filter.
     * @param[in] type             Tyep of the filter: LPF, HPF, BPF, NOTCH (from enum FilterType).
     * @param[in] iOrder           Represents the order of the filter, the higher the higher is the stopband attenuation.
     * @param[in] dCenterfreq      Determines the center of the frequency - normed to sFreq/2 (nyquist).
     * @param[in] dBandwidth       Ignored if FilterType is set to LPF,HPF. if NOTCH/BPF: bandwidth of stop-/passband - normed to sFreq/2 (nyquist).
     * @param[in] dParkswidth      Determines the width of the filter slopes (steepness) - normed to sFreq/2 (nyquist).
     * @param[in] dSFreq           The sampling frequency.
     * @param[in] designMethod     Specifies the design method to use. Choose between Cosind and Tschebyscheff.
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
     * @param[in] iDataSize           The data size to setup the filters to.
     */
    void prepareFilter(int iDataSize);

    //=========================================================================================================
    /**
     * Applies the current filter to the input data using convolution in time domain.
     *
     * @param[in] vecData          Holds the data to be filtered.
     * @param[in] bKeepOverhead    Whether the result should still include the overhead information in front and back of the data.
     *                              Default is set to false.
     *
     * @return the filtered data in form of a RowVectorXd.
     */
    Eigen::RowVectorXd applyConvFilter(const Eigen::RowVectorXd& vecData,
                                       bool bKeepOverhead = false) const;

    //=========================================================================================================
    /**
     * Applies the current filter to the input data using multiplication in frequency domain.
     *
     * @param[in, out] vecData              Holds the data to be filtered. Gets overwritten with its filtered result.
     * @param[in] bKeepOverhead            Whether the result should still include the overhead information in front and back of the data.
     *                                      Default is set to false.
     *
     * @return the filtered data in form of a RowVectorXd.
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

    FilterParameter getDesignMethod() const;
    void setDesignMethod(int iDesignMethod);

    FilterParameter getFilterType() const;
    void setFilterType(int iFilterType);

    QString getShortDescription() const;

    static QVector<FilterParameter> m_designMethods;  /**< Vector of possible filter design methods. */
    static QVector<FilterParameter> m_filterTypes;    /**< Vector of possible filter design types. */

private:
    //=========================================================================================================
    /**
     * Transforms the calculated filter coefficients to frequency-domain
     *
     * @param[in] iFftLength                  The FFT length to use.
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
    double          m_dHighpassFreq;        /**< highpass freq (lower cut off) of the filter. */

    int             m_iFilterOrder;         /**< represents the order of the filter instance. */
    int             m_iDesignMethod;        /**< represents the design method of the filter instance.*/
    int             m_iFilterType;          /**< represents the type of the filter instance.*/

    QString         m_sFilterName;          /**< contains name of the filter. */
    QString         m_sFilterShortDescription; /**< contains a short string describign some filter parameters. */

    Eigen::RowVectorXd     m_vecCoeff;       /**< contains the forward filter coefficient set. */
    Eigen::RowVectorXcd    m_vecFftCoeff;    /**< the FFT-transformed forward filter coefficient set, required for frequency-domain filtering, zero-padded to m_iFftLength. */
};

} // NAMESPACE UTILSLIB

#ifndef metatype_filterkernel
#define metatype_filterkernel
Q_DECLARE_METATYPE(UTILSLIB::FilterKernel)
#endif

#ifndef metatype_filterparameter
#define metatype_filterkernel
Q_DECLARE_METATYPE(UTILSLIB::FilterParameter)
#endif

#endif // FILTERKERNEL_H
