//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sessionfilter.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     March, 2026
 * @version  2.1.0
 * @brief    Shared session filter definition used by the mne_browse raw browser preview and
 *           offline processing workflows.
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
