//=============================================================================================================
/**
 * @file     iirfilter.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
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
 * @brief    Implementation of IirFilter.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "iirfilter.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// C++ INCLUDES
//=============================================================================================================

#include <cmath>
#include <complex>
#include <algorithm>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// CONSTANTS
//=============================================================================================================

static constexpr double PI = M_PI;

//=============================================================================================================
// PRIVATE HELPERS
//=============================================================================================================

namespace {

//=============================================================================================================
/**
 * @brief Evaluate the magnitude of H(z) at z = e^{j*w}, where w is in rad/sample.
 */
double evalMagnitude(const QVector<IirBiquad>& sos, double omega)
{
    std::complex<double> z(std::cos(omega), std::sin(omega));
    std::complex<double> H(1.0, 0.0);
    std::complex<double> z1(1.0, 0.0);    // z^{-1} placeholder

    for (const IirBiquad& bq : sos) {
        std::complex<double> zinv  = 1.0 / z;
        std::complex<double> zinv2 = zinv * zinv;
        std::complex<double> num = bq.b0 + bq.b1 * zinv + bq.b2 * zinv2;
        std::complex<double> den = 1.0   + bq.a1 * zinv + bq.a2 * zinv2;
        H *= num / den;
    }
    return std::abs(H);
}

} // anonymous namespace

//=============================================================================================================
// MEMBER DEFINITIONS
//=============================================================================================================

QVector<std::complex<double>> IirFilter::butterworthPrototypePoles(int n)
{
    // Butterworth analogue LP prototype poles (normalised cutoff = 1 rad/s):
    // p_k = exp( j * PI * (2k - 1 + n) / (2n) )  for k = 1 .. n
    // All poles lie on the unit circle in the left half-plane.
    QVector<std::complex<double>> poles;
    poles.reserve(n);
    for (int k = 1; k <= n; ++k) {
        double angle = PI * static_cast<double>(2 * k - 1 + n) / static_cast<double>(2 * n);
        poles.append(std::complex<double>(std::cos(angle), std::sin(angle)));
    }
    return poles;
}

//=============================================================================================================

IirBiquad IirFilter::poleToDigitalBiquad(std::complex<double> pole,
                                           double               dC,
                                           double               dGain)
{
    // Given complex conjugate pole pair (pole, conj(pole)), the analogue biquad is:
    //   H_a(s) = gain * omega0^2 / (s^2 - 2*Re(pole)*s + |pole|^2)
    //   where omega0 = |pole|
    //
    // Bilinear transform s = dC*(z-1)/(z+1):
    // Let omega0 = |pole|, alpha = -Re(pole) (> 0), c = dC
    //
    //   H_a(s) numerator coefficients: b2_a=0, b1_a=0, b0_a = gain*omega0^2
    //   H_a(s) denominator:            a2_a=1, a1_a = 2*alpha, a0_a = omega0^2
    //
    // After bilinear (general formula for b2_a=0, b1_a=0):
    //   d0 = c^2 + 2*alpha*c + omega0^2
    //   b0 = gain * omega0^2 / d0
    //   b1 = 2 * gain * omega0^2 / d0
    //   b2 = gain * omega0^2 / d0
    //   a1 = 2*(omega0^2 - c^2) / d0
    //   a2 = (c^2 - 2*alpha*c + omega0^2) / d0

    double omega0 = std::abs(pole);
    double alpha  = -pole.real();   // positive for stable (left-half-plane) poles

    double omega0sq = omega0 * omega0;
    double d0 = dC * dC + 2.0 * alpha * dC + omega0sq;

    IirBiquad bq;
    bq.b0 =  dGain * omega0sq / d0;
    bq.b1 =  2.0 * dGain * omega0sq / d0;
    bq.b2 =  dGain * omega0sq / d0;
    bq.a1 =  2.0 * (omega0sq - dC * dC) / d0;
    bq.a2 =  (dC * dC - 2.0 * alpha * dC + omega0sq) / d0;
    return bq;
}

//=============================================================================================================

IirBiquad IirFilter::realPoleToDigitalSection(double dPoleReal, double dC, double dGain)
{
    // H_a(s) = gain * |pole| / (s - dPoleReal)   (dPoleReal < 0)
    // Bilinear: s = dC*(z-1)/(z+1)
    //   H(z) = gain * |pole| * (1 + z^{-1}) / ((dC + |pole|) - (dC - |pole|)*z^{-1})
    //         = [gain*|pole|/(dC+|pole|)] * (1 + z^{-1}) / (1 - [(dC-|pole|)/(dC+|pole|)]*z^{-1})

    double absPole = std::abs(dPoleReal);
    double sum     = dC + absPole;

    IirBiquad bq;
    bq.b0 = dGain * absPole / sum;
    bq.b1 = dGain * absPole / sum;
    bq.b2 = 0.0;
    bq.a1 = -(dC - absPole) / sum;
    bq.a2 = 0.0;
    return bq;
}

//=============================================================================================================

QVector<IirBiquad> IirFilter::designButterworth(int        iOrder,
                                                  FilterType type,
                                                  double     dCutoffLow,
                                                  double     dCutoffHigh,
                                                  double     dSFreq)
{
    if (iOrder < 1) {
        qWarning() << "IirFilter::designButterworth: order must be >= 1, got" << iOrder;
        return {};
    }

    const double dC = 2.0 * dSFreq;   // bilinear pre-warp constant

    // Pre-warp analogue cutoff frequencies
    double dOmegaLow  = dC * std::tan(PI * dCutoffLow  / dSFreq);
    double dOmegaHigh = (type == BandPass || type == BandStop)
                        ? dC * std::tan(PI * dCutoffHigh / dSFreq)
                        : 0.0;

    // Butterworth normalised LP prototype poles
    QVector<std::complex<double>> protoPoles = butterworthPrototypePoles(iOrder);

    QVector<IirBiquad> sos;

    if (type == LowPass || type == HighPass) {
        // ---- LP / HP -------------------------------------------------------
        // LP: scale pole by Omega_c  →  p_k_lp = Omega_c * proto_k
        // HP: invert-and-scale      →  p_k_hp = Omega_c / proto_k
        //
        // Process complex conjugate pairs (take poles with Im >= 0).
        // If order is odd there will be one real pole at proto = -1.

        for (int k = 0; k < iOrder; ++k) {
            std::complex<double> proto = protoPoles[k];

            // Skip the conjugate (imaginary part < 0); it is handled together with its pair.
            if (proto.imag() < -1e-10) {
                continue;
            }

            if (std::abs(proto.imag()) < 1e-10) {
                // Real pole (only for odd order, proto ≈ -1)
                double analogPole = (type == LowPass)
                                    ? dOmegaLow * proto.real()
                                    : dOmegaLow / proto.real();

                // Evaluate passband gain at DC (LP) or Nyquist (HP) for normalisation.
                // For a single first-order section: |H(0)| = |b0+b1|/|1+a1|
                // We design with dGain=1 first, then normalise.
                IirBiquad bq = realPoleToDigitalSection(analogPole, dC, 1.0);

                // Normalise so passband gain ~ 1
                double w_pb = (type == LowPass) ? 0.0 : PI;
                double num = std::abs(bq.b0 + bq.b1 * std::cos(-w_pb));  // rough, imaginary part 0
                double den = std::abs(1.0   + bq.a1 * std::cos(-w_pb));
                double gain = (den > 1e-12 && num > 1e-12) ? den / num : 1.0;
                bq.b0 *= gain;
                bq.b1 *= gain;

                sos.append(bq);
            } else {
                // Complex conjugate pair
                std::complex<double> analogPole;
                double sectionGain;

                if (type == LowPass) {
                    analogPole  = dOmegaLow * proto;
                    sectionGain = dOmegaLow * dOmegaLow;   // LP: constant numerator = omega0^2
                } else {
                    // HP: s -> Omega_c / s  transforms pole p to Omega_c / p
                    analogPole  = dOmegaLow / proto;
                    sectionGain = 1.0;                      // HP: numerator has s^2 -> gain handled below
                }

                if (type == LowPass) {
                    IirBiquad bq = poleToDigitalBiquad(analogPole, dC, sectionGain);
                    // Normalise: LP passband gain at DC (z=1, omega=0) should be 1
                    // At z=1: H = (b0+b1+b2)/(1+a1+a2)
                    double hDC = (bq.b0 + bq.b1 + bq.b2) / (1.0 + bq.a1 + bq.a2);
                    if (std::abs(hDC) > 1e-12) {
                        double scale = 1.0 / std::abs(hDC);
                        bq.b0 *= scale; bq.b1 *= scale; bq.b2 *= scale;
                    }
                    sos.append(bq);
                } else {
                    // HP biquad: analogue section H(s) = s^2 / (s^2 - 2*Re(p)*s + |p|^2)
                    // b2_a=1, b1_a=0, b0_a=0; a2_a=1, a1_a=-2*Re(p), a0_a=|p|^2
                    // General bilinear for (b2_a=1, b1_a=0, b0_a=0):
                    double omega0 = std::abs(analogPole);
                    double alpha  = -analogPole.real();
                    double omega0sq = omega0 * omega0;
                    double d0 = dC * dC + 2.0 * alpha * dC + omega0sq;

                    IirBiquad bq;
                    bq.b0 =  dC * dC / d0;
                    bq.b1 = -2.0 * dC * dC / d0;
                    bq.b2 =  dC * dC / d0;
                    bq.a1 =  2.0 * (omega0sq - dC * dC) / d0;
                    bq.a2 =  (dC * dC - 2.0 * alpha * dC + omega0sq) / d0;

                    // Normalise: HP passband gain at Nyquist (z=-1, omega=pi) should be 1
                    // At z=-1: H = (b0-b1+b2)/(1-a1+a2)
                    double hNy = (bq.b0 - bq.b1 + bq.b2) / (1.0 - bq.a1 + bq.a2);
                    if (std::abs(hNy) > 1e-12) {
                        double scale = 1.0 / std::abs(hNy);
                        bq.b0 *= scale; bq.b1 *= scale; bq.b2 *= scale;
                    }
                    sos.append(bq);
                }
            }
        }

    } else {
        // ---- BP / BS -------------------------------------------------------
        // LP prototype pole p_k → BP/BS analogue poles via:
        //   BP: s^2 - Bw*p_k*s + Omega0^2 = 0
        //   BS: Bw*s / (s^2 + Omega0^2) = p_k  (i.e., solve s^2 - (Bw/p_k)*s + Omega0^2 = 0)
        // This doubles the order, producing 2*iOrder poles → iOrder biquad sections.

        double dOmega0 = std::sqrt(dOmegaLow * dOmegaHigh);
        double dBw     = dOmegaHigh - dOmegaLow;

        for (int k = 0; k < iOrder; ++k) {
            std::complex<double> proto = protoPoles[k];

            // Solve quadratic for the two BP or BS analogue poles from this prototype pole.
            // BP quadratic: s^2 - Bw*proto*s + Omega0^2 = 0
            // BS quadratic: s^2 - (Bw/proto)*s + Omega0^2 = 0
            std::complex<double> mid;
            if (type == BandPass) {
                mid = dBw * proto;
            } else {
                mid = dBw / proto;
            }

            std::complex<double> discriminant = mid * mid - 4.0 * dOmega0 * dOmega0;
            std::complex<double> sqrtDisc     = std::sqrt(discriminant);
            std::complex<double> pole1 = (mid + sqrtDisc) / 2.0;
            std::complex<double> pole2 = (mid - sqrtDisc) / 2.0;

            // Each of pole1, pole2 forms a complex conjugate pair with its own conjugate.
            // Design one biquad section per pole (using poleToDigitalBiquad which handles
            // the conjugate pair automatically).

            for (auto& pole : {pole1, pole2}) {
                if (type == BandPass) {
                    // BP analogue biquad numerator: Bw*s  (zero at origin, zero at infinity)
                    // i.e., b2_a=0, b1_a=Bw, b0_a=0
                    // Bilinear of (b2_a=0, b1_a=Bw, b0_a=0) / (s^2 - 2Re(p)s + |p|^2):
                    double omega0  = std::abs(pole);
                    double alpha   = -pole.real();
                    double omega0sq = omega0 * omega0;
                    double d0 = dC * dC + 2.0 * alpha * dC + omega0sq;

                    IirBiquad bq;
                    // num from bilinear(b1_a=Bw): b0=Bw*c, b1=0, b2=-Bw*c
                    bq.b0 =  dBw * dC / d0;
                    bq.b1 =  0.0;
                    bq.b2 = -dBw * dC / d0;
                    bq.a1 =  2.0 * (omega0sq - dC * dC) / d0;
                    bq.a2 =  (dC * dC - 2.0 * alpha * dC + omega0sq) / d0;
                    sos.append(bq);
                } else {
                    // BS analogue biquad numerator: s^2 + Omega0^2  (zeros at ±j*Omega0)
                    // b2_a=1, b1_a=0, b0_a=Omega0^2
                    // Bilinear:
                    double omega0  = std::abs(pole);
                    double alpha   = -pole.real();
                    double omega0sq = omega0 * omega0;
                    double dOmega0sq = dOmega0 * dOmega0;
                    double d0 = dC * dC + 2.0 * alpha * dC + omega0sq;

                    IirBiquad bq;
                    // num from bilinear(b2_a=1, b1_a=0, b0_a=Omega0^2):
                    //   b0 = (c^2 + Omega0^2)/d0, b1 = 2*(Omega0^2-c^2)/d0, b2 = (c^2+Omega0^2)/d0
                    bq.b0 = (dC * dC + dOmega0sq) / d0;
                    bq.b1 =  2.0 * (dOmega0sq - dC * dC) / d0;
                    bq.b2 = (dC * dC + dOmega0sq) / d0;
                    bq.a1 =  2.0 * (omega0sq - dC * dC) / d0;
                    bq.a2 =  (dC * dC - 2.0 * alpha * dC + omega0sq) / d0;
                    sos.append(bq);
                }
            }
        }

        // Normalise overall passband gain
        // For BP: evaluate at centre frequency Omega0 (digital: omega_0 = Omega0 / fs * 2pi... use prewarped)
        // For BS: evaluate at DC (omega = 0)
        double omegaCheck = (type == BandPass)
                            ? 2.0 * std::atan(dOmega0 / dC)   // bilinear inverse: omega_d = 2*atan(Omega_a/c)
                            : 0.0;
        double totalGain = evalMagnitude(sos, omegaCheck);
        if (totalGain > 1e-12) {
            double scale = 1.0 / totalGain;
            // Distribute scale evenly across sections
            double perSection = std::pow(scale, 1.0 / sos.size());
            for (IirBiquad& bq : sos) {
                bq.b0 *= perSection;
                bq.b1 *= perSection;
                bq.b2 *= perSection;
            }
        }
    }

    return sos;
}

//=============================================================================================================

RowVectorXd IirFilter::applySos(const RowVectorXd& vecData, const QVector<IirBiquad>& sos)
{
    if (sos.isEmpty() || vecData.size() == 0) {
        return vecData;
    }

    RowVectorXd y = vecData;

    // Cascade all biquad sections using Direct-Form II transposed structure:
    // w[n] = x[n] - a1*w[n-1] - a2*w[n-2]
    // y[n] = b0*w[n] + b1*w[n-1] + b2*w[n-2]
    // Equivalent transposed: uses two state variables (s1, s2)

    for (const IirBiquad& bq : sos) {
        RowVectorXd x = y;
        double s1 = 0.0, s2 = 0.0;

        for (int i = 0; i < static_cast<int>(x.size()); ++i) {
            double xi = x(i);
            double yi = bq.b0 * xi + s1;
            s1 = bq.b1 * xi - bq.a1 * yi + s2;
            s2 = bq.b2 * xi - bq.a2 * yi;
            y(i) = yi;
        }
    }

    return y;
}

//=============================================================================================================

RowVectorXd IirFilter::applyZeroPhase(const RowVectorXd& vecData, const QVector<IirBiquad>& sos)
{
    if (sos.isEmpty() || vecData.size() == 0) {
        return vecData;
    }

    // Forward pass
    RowVectorXd y = applySos(vecData, sos);

    // Reverse
    RowVectorXd yRev = y.reverse();

    // Backward pass
    RowVectorXd yRevFilt = applySos(yRev, sos);

    // Reverse again
    return yRevFilt.reverse();
}

//=============================================================================================================

MatrixXd IirFilter::applyZeroPhaseMatrix(const MatrixXd& matData, const QVector<IirBiquad>& sos)
{
    if (sos.isEmpty() || matData.size() == 0) {
        return matData;
    }

    MatrixXd matOut(matData.rows(), matData.cols());
    for (int i = 0; i < static_cast<int>(matData.rows()); ++i) {
        matOut.row(i) = applyZeroPhase(matData.row(i), sos);
    }
    return matOut;
}
