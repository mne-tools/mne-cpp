//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     filterkernel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Linear-phase FIR filter kernel with overlap-add FFT convolution back-end.
 *
 * FilterKernel is the core FIR engine of the DSP library: it owns the impulse
 * response coefficients of a low-pass, high-pass, band-pass or notch filter
 * together with their zero-padded FFT and applies them to incoming data via
 * overlap-add convolution. Two design back-ends are supported — the
 * cosine-tapered raised-cosine design (@ref mne::CosineFilter, fast, smooth
 * roll-off) and the Parks–McClellan equiripple design
 * (@ref mne::ParksMcClellan, optimal minimax behaviour). Both produce Type-I
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

#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Named filter-design parameter descriptor holding a human-readable name and description (e.g. design method or filter type).
 *
 * @snippet ex_dsp_filter/main.cpp filter_kernel_design
 */
class DSPSHARED_EXPORT FilterParameter
{
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
    explicit FilterParameter(std::string sName);

    //=========================================================================================================
    /**
     * Creates a filter parameter with name sName and description sDescription
     *
     * @param[in] sName            input name.
     * @param[in] sDescription     input desxription.
     */
    explicit FilterParameter(std::string sName, std::string sDescription);

    //=========================================================================================================
    /**
     * Returns parameter name
     *
     * @return Parameter name.
     */
    const std::string& name() const;

    friend bool operator==(const FilterParameter& in1, const FilterParameter& in2)
    {
        return in1.name() == in2.name();
    }

protected:
    std::string m_sName;        /**< Item name. */
    std::string m_sDescription; /**< Item description. */
};

//=============================================================================================================
/**
 * The FilterKernel class provides methods to create/design a FIR filter kernel
 *
 * @brief The FilterKernel class provides methods to create/design a FIR filter kernel
 *
 * @snippet ex_dsp_filter/main.cpp filter_kernel_design
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
     * @param[in] iFilterType             Tyep of the filter: LPF, HPF, BPF, NOTCH (from enum FilterType).
     * @param[in] iOrder           Represents the order of the filter, the higher the higher is the stopband attenuation.
     * @param[in] dCenterfreq      Determines the center of the frequency - normed to sFreq/2 (nyquist).
     * @param[in] dBandwidth       Ignored if FilterType is set to LPF,HPF. if NOTCH/BPF: bandwidth of stop-/passband - normed to sFreq/2 (nyquist).
     * @param[in] dParkswidth      Determines the width of the filter slopes (steepness) - normed to sFreq/2 (nyquist).
     * @param[in] dSFreq           The sampling frequency.
     * @param[in] iDesignMethod     Specifies the design method to use. Choose between Cosind and Tschebyscheff.
     **/
    FilterKernel(const std::string& sFilterName,
                 int iFilterType,
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
    void prepare_filter(int iDataSize);

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
    Eigen::RowVectorXd apply_conv_filter(const Eigen::RowVectorXd& vecData,
                                         bool bKeepOverhead = false) const;

    //=========================================================================================================
    /**
     * Applies the current filter to the input data using multiplication in frequency domain.
     *
     * @param[in, out] vecData              Holds the data to be filtered. Gets overwritten with its filtered result.
     * @param[in] bKeepOverhead            Whether the result should still include the overhead information in front and back of the data.
     *                                      Default is set to false.
     */
    void apply_fft_filter(Eigen::RowVectorXd& vecData,
                          bool bKeepOverhead = false);

    const std::string& name() const;
    void set_name(std::string sFilterName);

    double sampling_frequency() const;
    void set_sampling_frequency(double dSFreq);

    int filter_order() const;
    void set_filter_order(int iOrder);

    double center_frequency() const;
    void set_center_frequency(double dCenterFreq);

    double bandwidth() const;
    void set_bandwidth(double dBandwidth);

    double parks_width() const;
    void set_parks_width(double dParksWidth);

    double highpass_freq() const;
    void set_highpass_freq(double dHighpassFreq);

    double lowpass_freq() const;
    void set_lowpass_freq(double dLowpassFreq);

    const Eigen::RowVectorXd& coefficients() const;
    void set_coefficients(const Eigen::RowVectorXd& vecCoeff);

    const Eigen::RowVectorXcd& fft_coefficients() const;
    void set_fft_coefficients(const Eigen::RowVectorXcd& vecFftCoeff);

    const FilterParameter& design_method() const;
    void set_design_method(int iDesignMethod);

    const FilterParameter& filter_type() const;
    void set_filter_type(int iFilterType);

    std::string short_description() const;

    //=========================================================================================================
    /**
     * @param[in] sName    A design method name, e.g. "Cosine".
     *
     * @return The index of the method in design_methods, or -1 if there is none of that name.
     */
    static int design_method_index(std::string_view sName);

    //=========================================================================================================
    /**
     * @param[in] sName    A filter type name, e.g. "BPF".
     *
     * @return The index of the type in filter_types, or -1 if there is none of that name.
     */
    static int filter_type_index(std::string_view sName);

    static const std::vector<FilterParameter> design_methods; /**< Vector of possible filter design methods. */
    static const std::vector<FilterParameter> filter_types;   /**< Vector of possible filter design types. */

private:
    //=========================================================================================================
    /**
     * Transforms the calculated filter coefficients to frequency-domain
     *
     * @param[in] iFftLength                  The FFT length to use.
     */
    bool fft_transform_coeffs(int iFftLength);

    //=========================================================================================================
    /**
     * Designs the actual filter with the given parameters
     */
    void design_filter();

    double m_sFreq;         /**< the sampling frequency. */
    double m_dCenterFreq;   /**< contains center freq of the filter. */
    double m_dBandwidth;    /**< contains bandwidth of the filter. */
    double m_dParksWidth;   /**< contains the parksmcallen width. */
    double m_dLowpassFreq;  /**< lowpass freq (higher cut off) of the filter. */
    double m_dHighpassFreq; /**< highpass freq (lower cut off) of the filter. */

    int m_iFilterOrder;  /**< represents the order of the filter instance. */
    int m_iDesignMethod; /**< represents the design method of the filter instance.*/
    int m_iFilterType;   /**< represents the type of the filter instance.*/

    std::string m_sFilterName; /**< contains name of the filter. */

    Eigen::RowVectorXd m_vecCoeff;     /**< contains the forward filter coefficient set. */
    Eigen::RowVectorXcd m_vecFftCoeff; /**< the FFT-transformed forward filter coefficient set, required for frequency-domain filtering, zero-padded to m_iFftLength. */
};

} // NAMESPACE mne_dsp

#endif // FILTERKERNEL_H
