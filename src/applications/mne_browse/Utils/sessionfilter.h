//=============================================================================================================
/**
 * @file     sessionfilter.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  2.1.0
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    Shared session filter definition used by the mne_browse raw browser preview and
 *           offline processing workflows.
 *
 */

#ifndef SESSIONFILTER_H
#define SESSIONFILTER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_info.h>

#include <dsp/filterkernel.h>
#include <dsp/iirfilter.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>
#include <QString>

//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{

//=============================================================================================================
/**
 * @brief Serializable filter definition shared between browser preview and session processing.
 */
class SessionFilter
{
public:
    //=========================================================================================================
    /**
     * @brief Filter design family.
     */
    enum class DesignMethod {
        Cosine,         /**< FIR cosine-window design. */
        Tschebyscheff,  /**< FIR Parks-McClellan / equiripple design. */
        Butterworth     /**< IIR Butterworth design using SOS sections. */
    };

    //=========================================================================================================
    /**
     * @brief Supported filter types.
     */
    enum class FilterType {
        LowPass,        /**< Low-pass filter. */
        HighPass,       /**< High-pass filter. */
        BandPass,       /**< Band-pass filter. */
        BandStop        /**< Band-stop / notch filter. */
    };

    SessionFilter();
    SessionFilter(const QString& sName,
                  DesignMethod designMethod,
                  FilterType filterType,
                  int iOrder,
                  double dCutoffLowHz,
                  double dCutoffHighHz,
                  double dTransitionHz,
                  double dSamplingFrequencyHz,
                  const QString& sApplyTo);

    //=========================================================================================================
    /**
     * @brief Returns true when the current configuration can be designed and applied.
     */
    bool isValid() const;

    //=========================================================================================================
    /**
     * @brief Returns true when the filter uses the FIR backend.
     */
    bool isFir() const;

    //=========================================================================================================
    /**
     * @brief Returns true when the filter uses the IIR backend.
     */
    bool isIir() const;

    //=========================================================================================================
    /**
     * @brief Returns a concise human-readable description.
     */
    QString displayName() const;

    //=========================================================================================================
    /**
     * @brief Returns the configured target scope such as All, MEG, or EEG.
     */
    QString applyTo() const;

    //=========================================================================================================
    /**
     * @brief Returns true when the filter applies to the given channel.
     */
    bool appliesToChannel(const FIFFLIB::FiffInfo& info,
                          int channelIndex) const;

    //=========================================================================================================
    /**
     * @brief Apply the filter to one row vector.
     */
    Eigen::RowVectorXd applyToVector(const Eigen::RowVectorXd& data) const;

    //=========================================================================================================
    /**
     * @brief Apply the filter to all matching rows of a data block while leaving stimulus channels untouched.
     */
    Eigen::MatrixXd applyToMatrix(const Eigen::MatrixXd& data,
                                  const FIFFLIB::FiffInfo& info) const;

    //=========================================================================================================
    /**
     * @brief Compute a magnitude response for plotting.
     *
     * @param[in] iPoints  Number of frequency bins between 0 and Nyquist.
     * @return Magnitude response of length iPoints.
     */
    Eigen::VectorXd magnitudeResponse(int iPoints = 2048) const;

    //=========================================================================================================
    /**
     * @brief Compute a wrapped phase response in degrees for plotting.
     *
     * @param[in] iPoints  Number of frequency bins between 0 and Nyquist.
     * @return Wrapped phase response in degrees of length iPoints.
     */
    Eigen::VectorXd phaseResponse(int iPoints = 2048) const;

    //=========================================================================================================
    /**
     * @brief Export the current filter coefficients for saving to a text file.
     */
    QString coefficientExportText() const;

    //=========================================================================================================
    /**
     * @brief Return the recommended response size hint for the UI.
     */
    int responseSizeHint() const;

    //=========================================================================================================
    /**
     * @brief Return a conservative overlap size in samples for chunked filtering workflows.
     */
    int recommendedPaddingSamples() const;

    DesignMethod designMethod() const;
    FilterType filterType() const;
    int order() const;
    double cutoffLowHz() const;
    double cutoffHighHz() const;
    double transitionHz() const;
    double samplingFrequencyHz() const;

private:
    void ensureDesigned(int iDataSizeHint = 0) const;
    UTILSLIB::FilterKernel createFirKernel() const;
    QVector<UTILSLIB::IirBiquad> createIirSections() const;
    Eigen::VectorXcd frequencyResponse(int iPoints) const;

    QString                         m_sName;                /**< Human-readable filter name. */
    DesignMethod                    m_designMethod;         /**< Filter design method. */
    FilterType                      m_filterType;           /**< Filter type. */
    int                             m_iOrder;               /**< FIR taps or IIR order. */
    double                          m_dCutoffLowHz;         /**< Lower cutoff in Hz. */
    double                          m_dCutoffHighHz;        /**< Upper cutoff in Hz. */
    double                          m_dTransitionHz;        /**< FIR transition width in Hz. */
    double                          m_dSamplingFrequencyHz; /**< Sampling frequency in Hz. */
    QString                         m_sApplyTo;             /**< Channel scope for this filter. */
    mutable UTILSLIB::FilterKernel  m_firKernel;            /**< Lazily prepared FIR kernel. */
    mutable QVector<UTILSLIB::IirBiquad> m_iirSections;     /**< Lazily designed IIR SOS cascade. */
    mutable bool                    m_bFirDesigned = false; /**< True once the FIR kernel matches this session-filter definition. */
    mutable bool                    m_bIirDesigned = false; /**< True once the IIR sections match this session-filter definition. */
    mutable int                     m_iPreparedDataSize = 0;/**< Last data size used to prepare the FIR kernel. */
};

} // namespace MNEBROWSE

#endif // SESSIONFILTER_H
