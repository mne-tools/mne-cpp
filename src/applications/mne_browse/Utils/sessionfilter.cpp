//=============================================================================================================
/**
 * @file     sessionfilter.cpp
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
 * @brief    Implementation of the shared session-filter definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sessionfilter.h"

#include <dsp/firfilter.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <unsupported/Eigen/FFT>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QTextStream>

//=============================================================================================================
// C++ INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <complex>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;
using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace Eigen;

namespace {

QString normalizedScope(const QString& scope)
{
    QString normalized = scope.trimmed().toUpper();
    return normalized.isEmpty() ? QStringLiteral("ALL") : normalized;
}

bool channelMatchesScope(const FiffInfo& info, int channelIndex, const QString& scope)
{
    const QString normalized = normalizedScope(scope);
    if(normalized == QLatin1String("ALL")) {
        return true;
    }

    if(channelIndex < 0 || channelIndex >= info.ch_names.size() || channelIndex >= info.chs.size()) {
        return false;
    }

    const QString channelName = info.ch_names.at(channelIndex).trimmed().toUpper();
    const int kind = info.chs.at(channelIndex).kind;

    if(normalized == QLatin1String("MEG")) {
        return kind == FIFFV_MEG_CH;
    }
    if(normalized == QLatin1String("EEG")) {
        return kind == FIFFV_EEG_CH;
    }
    if(normalized == QLatin1String("ECG")) {
        return kind == FIFFV_ECG_CH || channelName.contains(QLatin1String("ECG"));
    }
    if(normalized == QLatin1String("EOG")) {
        return kind == FIFFV_EOG_CH || channelName.contains(QLatin1String("EOG"));
    }
    if(normalized == QLatin1String("EMG")) {
        return kind == FIFFV_EMG_CH || channelName.contains(QLatin1String("EMG"));
    }

    return channelName.contains(normalized);
}

QString filterTypeText(SessionFilter::FilterType type)
{
    switch(type) {
        case SessionFilter::FilterType::LowPass:
            return QStringLiteral("Low-pass");
        case SessionFilter::FilterType::HighPass:
            return QStringLiteral("High-pass");
        case SessionFilter::FilterType::BandPass:
            return QStringLiteral("Band-pass");
        case SessionFilter::FilterType::BandStop:
            return QStringLiteral("Band-stop");
    }

    return QStringLiteral("Filter");
}

QString designMethodText(SessionFilter::DesignMethod method)
{
    switch(method) {
        case SessionFilter::DesignMethod::Cosine:
            return QStringLiteral("Cosine FIR");
        case SessionFilter::DesignMethod::Tschebyscheff:
            return QStringLiteral("Tschebyscheff FIR");
        case SessionFilter::DesignMethod::Butterworth:
            return QStringLiteral("Butterworth IIR");
    }

    return QStringLiteral("Filter");
}

int nextPowerOfTwo(int value)
{
    int power = 1;
    while(power < value) {
        power <<= 1;
    }

    return power;
}

std::complex<double> evalIirResponse(const QVector<IirBiquad>& sos, double omega)
{
    std::complex<double> z(std::cos(omega), std::sin(omega));
    std::complex<double> response(1.0, 0.0);

    for(const IirBiquad& biquad : sos) {
        const std::complex<double> zinv = 1.0 / z;
        const std::complex<double> zinv2 = zinv * zinv;
        const std::complex<double> numerator = biquad.b0 + biquad.b1 * zinv + biquad.b2 * zinv2;
        const std::complex<double> denominator = 1.0 + biquad.a1 * zinv + biquad.a2 * zinv2;
        response *= numerator / denominator;
    }

    return response;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SessionFilter::SessionFilter()
: m_sName(QStringLiteral("Session Filter"))
, m_designMethod(DesignMethod::Cosine)
, m_filterType(FilterType::LowPass)
, m_iOrder(0)
, m_dCutoffLowHz(0.0)
, m_dCutoffHighHz(0.0)
, m_dTransitionHz(0.0)
, m_dSamplingFrequencyHz(0.0)
, m_sApplyTo(QStringLiteral("All"))
{
}

//=============================================================================================================

SessionFilter::SessionFilter(const QString& sName,
                             DesignMethod designMethod,
                             FilterType filterType,
                             int iOrder,
                             double dCutoffLowHz,
                             double dCutoffHighHz,
                             double dTransitionHz,
                             double dSamplingFrequencyHz,
                             const QString& sApplyTo)
: m_sName(sName)
, m_designMethod(designMethod)
, m_filterType(filterType)
, m_iOrder(iOrder)
, m_dCutoffLowHz(dCutoffLowHz)
, m_dCutoffHighHz(dCutoffHighHz)
, m_dTransitionHz(dTransitionHz)
, m_dSamplingFrequencyHz(dSamplingFrequencyHz)
, m_sApplyTo(sApplyTo)
{
}

//=============================================================================================================

bool SessionFilter::isValid() const
{
    if(m_dSamplingFrequencyHz <= 0.0 || m_iOrder <= 0) {
        return false;
    }

    const double nyquist = m_dSamplingFrequencyHz / 2.0;
    if(m_dCutoffLowHz <= 0.0 || m_dCutoffLowHz >= nyquist) {
        return false;
    }

    if((m_filterType == FilterType::BandPass || m_filterType == FilterType::BandStop)
       && (m_dCutoffHighHz <= m_dCutoffLowHz || m_dCutoffHighHz >= nyquist)) {
        return false;
    }

    if(isFir() && m_dTransitionHz <= 0.0) {
        return false;
    }

    return true;
}

//=============================================================================================================

bool SessionFilter::isFir() const
{
    return m_designMethod != DesignMethod::Butterworth;
}

//=============================================================================================================

bool SessionFilter::isIir() const
{
    return m_designMethod == DesignMethod::Butterworth;
}

//=============================================================================================================

QString SessionFilter::displayName() const
{
    QString rangeText;
    if(m_filterType == FilterType::LowPass || m_filterType == FilterType::HighPass) {
        rangeText = QString::number(m_dCutoffLowHz, 'f', 2) + QStringLiteral(" Hz");
    } else {
        rangeText = QStringLiteral("%1-%2 Hz")
                        .arg(QString::number(m_dCutoffLowHz, 'f', 2),
                             QString::number(m_dCutoffHighHz, 'f', 2));
    }

    return QStringLiteral("%1 %2 (%3)")
        .arg(filterTypeText(m_filterType),
             rangeText,
             designMethodText(m_designMethod));
}

//=============================================================================================================

QString SessionFilter::applyTo() const
{
    return m_sApplyTo;
}

//=============================================================================================================

bool SessionFilter::appliesToChannel(const FiffInfo& info, int channelIndex) const
{
    if(channelIndex < 0 || channelIndex >= info.chs.size()) {
        return false;
    }

    if(info.chs.at(channelIndex).kind == FIFFV_STIM_CH) {
        return false;
    }

    return channelMatchesScope(info, channelIndex, m_sApplyTo);
}

//=============================================================================================================

FilterKernel SessionFilter::createFirKernel() const
{
    FirFilter::FilterType type = FirFilter::BandPass;
    switch(m_filterType) {
        case FilterType::LowPass:
            type = FirFilter::LowPass;
            break;
        case FilterType::HighPass:
            type = FirFilter::HighPass;
            break;
        case FilterType::BandPass:
            type = FirFilter::BandPass;
            break;
        case FilterType::BandStop:
            type = FirFilter::BandStop;
            break;
    }

    FirFilter::DesignMethod method = m_designMethod == DesignMethod::Tschebyscheff
        ? FirFilter::ParksMcClellan
        : FirFilter::Cosine;

    return FirFilter::design(m_iOrder,
                             type,
                             m_dCutoffLowHz,
                             m_dCutoffHighHz,
                             m_dSamplingFrequencyHz,
                             m_dTransitionHz,
                             method);
}

//=============================================================================================================

QVector<IirBiquad> SessionFilter::createIirSections() const
{
    IirFilter::FilterType type = IirFilter::BandPass;
    switch(m_filterType) {
        case FilterType::LowPass:
            type = IirFilter::LowPass;
            break;
        case FilterType::HighPass:
            type = IirFilter::HighPass;
            break;
        case FilterType::BandPass:
            type = IirFilter::BandPass;
            break;
        case FilterType::BandStop:
            type = IirFilter::BandStop;
            break;
    }

    return IirFilter::designButterworth(m_iOrder,
                                        type,
                                        m_dCutoffLowHz,
                                        m_dCutoffHighHz,
                                        m_dSamplingFrequencyHz);
}

//=============================================================================================================

void SessionFilter::ensureDesigned(int iDataSizeHint) const
{
    if(!isValid()) {
        return;
    }

    if(isFir()) {
        if(!m_bFirDesigned) {
            m_firKernel = createFirKernel();
            m_iPreparedDataSize = 0;
            m_bFirDesigned = true;
        }

        if(iDataSizeHint > 0 && iDataSizeHint != m_iPreparedDataSize) {
            m_firKernel.prepareFilter(iDataSizeHint);
            m_iPreparedDataSize = iDataSizeHint;
        }
    } else if(!m_bIirDesigned) {
        m_iirSections = createIirSections();
        m_bIirDesigned = true;
    }
}

//=============================================================================================================

RowVectorXd SessionFilter::applyToVector(const RowVectorXd& data) const
{
    if(!isValid() || data.size() == 0) {
        return data;
    }

    ensureDesigned(static_cast<int>(data.size()));

    if(isFir()) {
        FilterKernel kernel = m_firKernel;
        return FirFilter::applyZeroPhase(data, kernel);
    }

    return IirFilter::applyZeroPhase(data, m_iirSections);
}

//=============================================================================================================

MatrixXd SessionFilter::applyToMatrix(const MatrixXd& data,
                                      const FiffInfo& info) const
{
    if(!isValid() || data.size() == 0) {
        return data;
    }

    ensureDesigned(static_cast<int>(data.cols()));

    MatrixXd filtered = data;
    for(int row = 0; row < filtered.rows(); ++row) {
        if(!appliesToChannel(info, row)) {
            continue;
        }

        if(row < info.chs.size() && info.chs.at(row).kind == FIFFV_STIM_CH) {
            continue;
        }

        filtered.row(row) = applyToVector(filtered.row(row));
    }

    return filtered;
}

//=============================================================================================================

VectorXcd SessionFilter::frequencyResponse(int iPoints) const
{
    VectorXcd response = VectorXcd::Zero(std::max(2, iPoints));
    if(!isValid()) {
        return response;
    }

    if(isFir()) {
        ensureDesigned();
        const RowVectorXd coefficients = m_firKernel.getCoefficients();
        if(coefficients.size() == 0) {
            return response;
        }

        const int coeffCount = static_cast<int>(coefficients.size());
        const int responseCount = static_cast<int>(response.size());
        const int fftLength = nextPowerOfTwo(std::max(coeffCount * 4,
                                                      responseCount * 2));
        RowVectorXd padded = RowVectorXd::Zero(fftLength);
        padded.head(coefficients.cols()) = coefficients;

        Eigen::FFT<double> fft;
        fft.SetFlag(fft.HalfSpectrum);

        RowVectorXcd fftCoefficients;
        fft.fwd(fftCoefficients, padded, fftLength);

        for(int index = 0; index < response.size(); ++index) {
            const double position = static_cast<double>(index) / static_cast<double>(response.size() - 1);
            const int spectrumIndex = std::min(static_cast<int>(std::round(position * (fftCoefficients.size() - 1))),
                                               static_cast<int>(fftCoefficients.size() - 1));
            response(index) = fftCoefficients(spectrumIndex);
        }

        return response;
    }

    ensureDesigned();
    for(int index = 0; index < response.size(); ++index) {
        const double omega = M_PI * static_cast<double>(index) / static_cast<double>(response.size() - 1);
        response(index) = evalIirResponse(m_iirSections, omega);
    }

    return response;
}

//=============================================================================================================

VectorXd SessionFilter::magnitudeResponse(int iPoints) const
{
    const VectorXcd complexResponse = frequencyResponse(iPoints);
    VectorXd magnitude(complexResponse.size());
    for(int index = 0; index < complexResponse.size(); ++index) {
        magnitude(index) = std::abs(complexResponse(index));
    }

    return magnitude;
}

//=============================================================================================================

VectorXd SessionFilter::phaseResponse(int iPoints) const
{
    const VectorXcd complexResponse = frequencyResponse(iPoints);
    VectorXd phase(complexResponse.size());
    for(int index = 0; index < complexResponse.size(); ++index) {
        phase(index) = std::arg(complexResponse(index)) * 180.0 / M_PI;
    }

    return phase;
}

//=============================================================================================================

QString SessionFilter::coefficientExportText() const
{
    if(!isValid()) {
        return QString();
    }

    ensureDesigned();

    QString output;
    QTextStream stream(&output);

    if(isFir()) {
        const RowVectorXd coefficients = m_firKernel.getCoefficients();
        for(Index index = 0; index < coefficients.cols(); ++index) {
            stream << coefficients(index) << '\n';
        }
        return output;
    }

    stream << "# section b0 b1 b2 a1 a2\n";
    for(int section = 0; section < m_iirSections.size(); ++section) {
        const IirBiquad& biquad = m_iirSections.at(section);
        stream << section << ' '
               << biquad.b0 << ' '
               << biquad.b1 << ' '
               << biquad.b2 << ' '
               << biquad.a1 << ' '
               << biquad.a2 << '\n';
    }

    return output;
}

//=============================================================================================================

int SessionFilter::responseSizeHint() const
{
    if(isFir()) {
        return nextPowerOfTwo(std::max(4096, m_iOrder * 4));
    }

    return std::max(1, static_cast<int>(std::ceil(m_iOrder / 2.0)));
}

//=============================================================================================================

int SessionFilter::recommendedPaddingSamples() const
{
    if(!isValid()) {
        return 0;
    }

    const int basePadding = static_cast<int>(std::ceil(m_dSamplingFrequencyHz));
    if(isFir()) {
        return std::max(basePadding, m_iOrder * 2);
    }

    return std::max(basePadding * 2, m_iOrder * 32);
}

//=============================================================================================================

SessionFilter::DesignMethod SessionFilter::designMethod() const
{
    return m_designMethod;
}

//=============================================================================================================

SessionFilter::FilterType SessionFilter::filterType() const
{
    return m_filterType;
}

//=============================================================================================================

int SessionFilter::order() const
{
    return m_iOrder;
}

//=============================================================================================================

double SessionFilter::cutoffLowHz() const
{
    return m_dCutoffLowHz;
}

//=============================================================================================================

double SessionFilter::cutoffHighHz() const
{
    return m_dCutoffHighHz;
}

//=============================================================================================================

double SessionFilter::transitionHz() const
{
    return m_dTransitionHz;
}

//=============================================================================================================

double SessionFilter::samplingFrequencyHz() const
{
    return m_dSamplingFrequencyHz;
}
