//=============================================================================================================
/**
 * @file     field_map.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     January, 2026
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
 * @brief    Sphere-model field mapping.
 *
 * The computations in this file are a C++ port of the MNE-Python field
 * mapping code, which is BSD-3-Clause licensed:
 *
 *     mne/forward/_lead_dots.py        – Legendre series & sphere dot products
 *     mne/forward/_field_interpolation.py – mapping matrix (SVD pseudo-inverse)
 *
 * Original Python authors: The MNE-Python contributors.
 * Copyright the MNE-Python contributors.
 * License: BSD-3-Clause
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "field_map.h"

#include <Eigen/SVD>
#include <algorithm>
#include <cmath>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FWDLIB;

//=============================================================================================================
// ANONYMOUS NAMESPACE – internal helpers
//=============================================================================================================

namespace
{

//=========================================================================
// Constants (matching MNE-Python mne/forward/_lead_dots.py)
//=========================================================================

/** Number of Legendre polynomial terms – matches MNE-Python "accurate" mode (n_coeff=100). */
constexpr int kNCoeff = 100;

/** mu_0^2 / (4*pi)  –  _meg_const in _lead_dots.py. */
constexpr double kMegConst = 4e-14 * M_PI;

/** 1 / (4*pi)  –  _eeg_const in _lead_dots.py. */
constexpr double kEegConst = 1.0 / (4.0 * M_PI);

/** EEG effective integration radius scale factor (applied inside _do_self_dots / _do_surface_dots). */
constexpr double kEegIntradScale = 0.7;

/** Ad-hoc noise standard deviations – matching MNE-Python make_ad_hoc_cov defaults. */
constexpr float kGradStd = 5e-13f;   // 5 fT/cm  (gradiometers)
constexpr float kMagStd  = 20e-15f;  // 20 fT    (magnetometers)
constexpr float kEegStd  = 1e-6f;    // 0.2 µV   (EEG, but value passed is 1 µV here matching _setup_dots)

//=========================================================================
// Legendre polynomials
// Port of _next_legen_der / _get_legen_der / _get_legen in _lead_dots.py
//=========================================================================

/**
 * Compute Legendre polynomials P_n(x) and their first and second
 * derivatives P_n'(x), P_n''(x) for n = 0 .. ncoeff-1.
 *
 * Matches the recurrence in MNE-Python _next_legen_der.
 */
void computeLegendreDer(double x, int ncoeff,
                        double* p, double* pd, double* pdd)
{
    p[0] = 1.0;  pd[0] = 0.0;  pdd[0] = 0.0;
    if (ncoeff < 2) return;
    p[1] = x;    pd[1] = 1.0;  pdd[1] = 0.0;
    for (int n = 2; n < ncoeff; ++n) {
        double old_p  = p[n - 1];
        double old_pd = pd[n - 1];
        p[n]   = ((2 * n - 1) * x * old_p - (n - 1) * p[n - 2]) / n;
        pd[n]  = n * old_p + x * old_pd;
        pdd[n] = (n + 1) * old_pd + x * pdd[n - 1];
    }
}

/**
 * Compute Legendre polynomials P_n(x) for n = 0 .. ncoeff-1.
 *
 * Matches legendre.legvander in MNE-Python (standard three-term recurrence).
 */
void computeLegendreVal(double x, int ncoeff, double* p)
{
    p[0] = 1.0;
    if (ncoeff < 2) return;
    p[1] = x;
    for (int n = 2; n < ncoeff; ++n) {
        p[n] = ((2 * n - 1) * x * p[n - 1] - (n - 1) * p[n - 2]) / n;
    }
}

//=========================================================================
// Series sums
// Port of _comp_sums_meg / _comp_sum_eeg in _lead_dots.py
//=========================================================================

/**
 * MEG Legendre series sums (surface integral, volume_integral = false).
 *
 * The four sums (for n = 1 .. kNCoeff-1):
 *   sums[0] = Σ beta^(n+1) · n(n+1)/(2n+1)       · P_n(ct)
 *   sums[1] = Σ beta^(n+1) · n/(2n+1)             · P_n'(ct)
 *   sums[2] = Σ beta^(n+1) · n/((2n+1)(n+1))      · P_n'(ct)
 *   sums[3] = Σ beta^(n+1) · n/((2n+1)(n+1))      · P_n''(ct)
 */
void compSumsMeg(double beta, double ctheta, double sums[4])
{
    double p[kNCoeff], pd[kNCoeff], pdd[kNCoeff];
    computeLegendreDer(ctheta, kNCoeff, p, pd, pdd);

    sums[0] = sums[1] = sums[2] = sums[3] = 0.0;
    double betan = beta;                         // accumulates beta^(n+1)
    for (int n = 1; n < kNCoeff; ++n) {
        betan *= beta;                           // beta^(n+1)
        double dn    = static_cast<double>(n);
        double multn = dn / (2.0 * dn + 1.0);   // n / (2n+1)
        double mult  = multn / (dn + 1.0);       // n / ((2n+1)(n+1))

        sums[0] += (dn + 1.0) * multn * p[n]   * betan;
        sums[1] +=               multn * pd[n]  * betan;
        sums[2] +=               mult  * pd[n]  * betan;
        sums[3] +=               mult  * pdd[n] * betan;
    }
}

/**
 * EEG Legendre series sum (for n = 1 .. kNCoeff-1):
 *   sum = Σ beta^n · (2n+1)^2 / n · P_n(ct)
 */
double compSumEeg(double beta, double ctheta)
{
    double p[kNCoeff];
    computeLegendreVal(ctheta, kNCoeff, p);

    double sum   = 0.0;
    double betan = 1.0;
    for (int n = 1; n < kNCoeff; ++n) {
        betan *= beta;                           // beta^n
        double dn     = static_cast<double>(n);
        double factor = 2.0 * dn + 1.0;
        sum += p[n] * betan * factor * factor / dn;
    }
    return sum;
}

//=========================================================================
// Sphere dot products
// Port of _fast_sphere_dot_r0 in _lead_dots.py
//=========================================================================

/**
 * MEG sphere dot product for two integration points.
 *
 * rr1, rr2  : normalised position vectors (relative to sphere origin)
 * lr1, lr2  : magnitudes of position vectors
 * cosmag1/2 : direction (cosmag) vectors
 */
double sphereDotMeg(double intrad,
                    const Vector3d& rr1, double lr1, const Vector3d& cosmag1,
                    const Vector3d& rr2, double lr2, const Vector3d& cosmag2)
{
    if (lr1 == 0.0 || lr2 == 0.0) return 0.0;

    double beta = (intrad * intrad) / (lr1 * lr2);
    double ct   = std::clamp(rr1.dot(rr2), -1.0, 1.0);

    double sums[4];
    compSumsMeg(beta, ct, sums);

    double n1c1 = cosmag1.dot(rr1);
    double n1c2 = cosmag1.dot(rr2);
    double n2c1 = cosmag2.dot(rr1);
    double n2c2 = cosmag2.dot(rr2);
    double n1n2 = cosmag1.dot(cosmag2);

    double part1 = ct * n1c1 * n2c2;
    double part2 = n1c1 * n2c1 + n1c2 * n2c2;

    double result = n1c1 * n2c2 * sums[0]
                  + (2.0 * part1 - part2) * sums[1]
                  + (n1n2 + part1 - part2) * sums[2]
                  + (n1c2 - ct * n1c1) * (n2c1 - ct * n2c2) * sums[3];

    result *= kMegConst / (lr1 * lr2);
    return result;
}

/**
 * EEG sphere dot product for two integration points.
 */
double sphereDotEeg(double intrad,
                    const Vector3d& rr1, double lr1,
                    const Vector3d& rr2, double lr2)
{
    if (lr1 == 0.0 || lr2 == 0.0) return 0.0;

    double beta = (intrad * intrad) / (lr1 * lr2);
    double ct   = std::clamp(rr1.dot(rr2), -1.0, 1.0);

    double sum = compSumEeg(beta, ct);
    return kEegConst * sum / (lr1 * lr2);
}

//=========================================================================
// Coil data extraction
// Port of the rmag/rlens/cosmags/ws extraction in _do_self_dots
//=========================================================================

/** Per-coil data: normalised positions relative to origin, magnitudes, etc. */
struct CoilData
{
    std::vector<Vector3d> rmag;     // normalised position vectors
    std::vector<double>   rlen;     // magnitudes
    std::vector<Vector3d> cosmag;   // direction vectors
    std::vector<double>   w;        // integration weights
    int np;                         // number of integration points
};

Vector3d toVec3d(const float* v) { return Vector3d(v[0], v[1], v[2]); }

/**
 * Extract & normalise coil integration-point data relative to sphere origin r0.
 *
 * Matches the "convert to normalised distances from expansion center" block
 * inside _do_self_dots / _do_surface_dots in _lead_dots.py.
 */
CoilData extractCoilData(const FwdCoil* coil, const Vector3d& r0)
{
    CoilData cd;
    cd.np = coil->np;
    cd.rmag.resize(cd.np);
    cd.rlen.resize(cd.np);
    cd.cosmag.resize(cd.np);
    cd.w.resize(cd.np);

    for (int i = 0; i < cd.np; ++i) {
        Vector3d rel = toVec3d(coil->rmag[i]) - r0;
        double len = rel.norm();
        cd.rmag[i]   = (len > 0.0) ? Vector3d(rel / len) : Vector3d::Zero();
        cd.rlen[i]   = len;
        cd.cosmag[i] = toVec3d(coil->cosmag[i]);
        cd.w[i]      = static_cast<double>(coil->w[i]);
    }
    return cd;
}

//=========================================================================
// Self-dot and surface-dot matrices
// Port of _do_self_dots / _do_surface_dots in _lead_dots.py
//=========================================================================

/**
 * Compute the sensor self-dot-product matrix (nchan × nchan, symmetric).
 *
 * Port of _do_self_dots in _lead_dots.py.
 * Uses double precision throughout to match MNE-Python (float64).
 */
MatrixXd doSelfDots(double intrad, const FwdCoilSet& coils, const Vector3d& r0, bool isMeg)
{
    const int nc = coils.ncoil;
    std::vector<CoilData> cdata(nc);
    for (int i = 0; i < nc; ++i) {
        cdata[i] = extractCoilData(coils.coils[i], r0);
    }

    MatrixXd products = MatrixXd::Zero(nc, nc);
    for (int ci1 = 0; ci1 < nc; ++ci1) {
        for (int ci2 = 0; ci2 <= ci1; ++ci2) {
            double dot = 0.0;
            const CoilData& c1 = cdata[ci1];
            const CoilData& c2 = cdata[ci2];
            for (int i = 0; i < c1.np; ++i) {
                for (int j = 0; j < c2.np; ++j) {
                    double ww = c1.w[i] * c2.w[j];
                    if (isMeg) {
                        dot += ww * sphereDotMeg(intrad,
                                   c1.rmag[i], c1.rlen[i], c1.cosmag[i],
                                   c2.rmag[j], c2.rlen[j], c2.cosmag[j]);
                    } else {
                        dot += ww * sphereDotEeg(intrad,
                                   c1.rmag[i], c1.rlen[i],
                                   c2.rmag[j], c2.rlen[j]);
                    }
                }
            }
            products(ci1, ci2) = dot;
            products(ci2, ci1) = dot;
        }
    }
    return products;
}

/**
 * Compute the surface-to-sensor dot-product matrix (nvert × nchan).
 *
 * For MEG, surface normals serve as cosmag1 (the "sensor direction" at each
 * surface vertex) and coil integration points provide cosmag2/weights.
 * Surface vertices have no integration weights (w1 = None in Python).
 *
 * Port of _do_surface_dots in _lead_dots.py.
 * Uses double precision throughout to match MNE-Python (float64).
 */
MatrixXd doSurfaceDots(double intrad, const FwdCoilSet& coils,
                       const MatrixX3f& rr, const MatrixX3f& nn,
                       const Vector3d& r0, bool isMeg)
{
    const int nc = coils.ncoil;
    const int nv = rr.rows();

    std::vector<CoilData> cdata(nc);
    for (int i = 0; i < nc; ++i) {
        cdata[i] = extractCoilData(coils.coils[i], r0);
    }

    MatrixXd products = MatrixXd::Zero(nv, nc);
    for (int vi = 0; vi < nv; ++vi) {
        // Surface vertex position relative to origin (normalised)
        Vector3d rel = rr.row(vi).cast<double>() - r0.transpose();
        double lsurf = rel.norm();
        Vector3d rsurf = (lsurf > 0.0) ? Vector3d(rel / lsurf) : Vector3d::Zero();
        Vector3d nsurf = nn.row(vi).cast<double>();  // surface normal (MEG cosmag)

        for (int ci = 0; ci < nc; ++ci) {
            const CoilData& c = cdata[ci];
            double dot = 0.0;
            for (int j = 0; j < c.np; ++j) {
                if (isMeg) {
                    dot += c.w[j] * sphereDotMeg(intrad,
                               rsurf, lsurf, nsurf,
                               c.rmag[j], c.rlen[j], c.cosmag[j]);
                } else {
                    dot += c.w[j] * sphereDotEeg(intrad,
                               rsurf, lsurf,
                               c.rmag[j], c.rlen[j]);
                }
            }
            products(vi, ci) = dot;
        }
    }
    return products;
}

//=========================================================================
// Ad-hoc noise standard deviations
// Matching MNE-Python make_ad_hoc_cov (called from _setup_dots)
//=========================================================================

/** MEG noise stds: magnetometers get kMagStd, gradiometers get kGradStd. */
VectorXd adHocMegStds(const FwdCoilSet& coils)
{
    VectorXd stds(coils.ncoil);
    for (int k = 0; k < coils.ncoil; ++k) {
        stds(k) = coils.coils[k]->is_axial_coil() ? static_cast<double>(kMagStd)
                                                   : static_cast<double>(kGradStd);
    }
    return stds;
}

/** EEG noise std: uniform kEegStd for all electrodes. */
VectorXd adHocEegStds(int ncoil)
{
    return VectorXd::Constant(ncoil, static_cast<double>(kEegStd));
}

//=========================================================================
// Mapping matrix computation
// Port of _compute_mapping_matrix / _pinv_trunc in _field_interpolation.py
//=========================================================================

/**
 * Compute the mapping matrix from sensor self-dots and surface-dots.
 *
 * All internal computation uses double precision to match MNE-Python (float64).
 * The final mapping matrix is returned in float for GPU-friendly rendering.
 *
 * Steps (matching MNE-Python _compute_mapping_matrix):
 *   1. Whiten: whitener = diag(1 / noiseStds)
 *      (In Python: whitener = diag(1/sqrt(noise_cov.data)) where data = std^2)
 *   2. whitened_dots = whitener @ self_dots @ whitener
 *   3. SVD → truncated pseudo-inverse (_pinv_trunc)
 *   4. inv_whitened = whitener @ inv @ whitener
 *   5. mapping = surface_dots @ inv_whitened
 *
 * Note: SSP projectors are not applied (proj_op = I).
 * Note: The average EEG reference projection is not applied.
 */
QSharedPointer<MatrixXf> computeMappingMatrix(const MatrixXd& selfDots,
                                              const MatrixXd& surfaceDots,
                                              const VectorXd& noiseStds,
                                              double miss)
{
    if (selfDots.rows() == 0 || surfaceDots.rows() == 0) {
        return QSharedPointer<MatrixXf>();
    }

    const int nchan = selfDots.rows();

    // Step 1: Build whitener = diag(1/std)
    VectorXd whitener(nchan);
    for (int i = 0; i < nchan; ++i) {
        whitener(i) = (noiseStds(i) > 0.0) ? (1.0 / noiseStds(i)) : 0.0;
    }

    // Step 2: whitened_dots = whitener @ self_dots @ whitener
    MatrixXd whitenedDots = whitener.asDiagonal() * selfDots * whitener.asDiagonal();

    // Step 3: SVD → truncated pseudo-inverse (port of _pinv_trunc)
    JacobiSVD<MatrixXd> svd(whitenedDots, ComputeFullU | ComputeFullV);
    VectorXd s = svd.singularValues();
    if (s.size() == 0 || s(0) <= 0.0) {
        return QSharedPointer<MatrixXf>();
    }

    // Eigenvalue truncation: keep components explaining >= (1-miss) of total variance
    // varexp = cumsum(s) / sum(s);  n = first index where varexp >= (1-miss), +1
    VectorXd varexp(s.size());
    varexp(0) = s(0);
    for (int i = 1; i < s.size(); ++i) {
        varexp(i) = varexp(i - 1) + s(i);
    }
    double totalVar = varexp(s.size() - 1);

    int n = s.size();  // keep all by default
    for (int i = 0; i < s.size(); ++i) {
        if (varexp(i) / totalVar >= (1.0 - miss)) {
            n = i + 1;
            break;
        }
    }

    // Pseudo-inverse: inv = V[:,:n] @ diag(1/s[:n]) @ U[:,:n]^T
    VectorXd sinv = VectorXd::Zero(s.size());
    for (int i = 0; i < n; ++i) {
        sinv(i) = (s(i) > 0.0) ? (1.0 / s(i)) : 0.0;
    }
    MatrixXd inv = svd.matrixV() * sinv.asDiagonal() * svd.matrixU().transpose();

    // Step 4: inv_whitened = whitener @ inv @ whitener
    MatrixXd invWhitened = whitener.asDiagonal() * inv * whitener.asDiagonal();

    // Step 5: mapping = surface_dots @ inv_whitened
    MatrixXd mapping = surfaceDots * invWhitened;

    // Convert to float for rendering
    MatrixXf mappingF = mapping.cast<float>();
    return QSharedPointer<MatrixXf>::create(std::move(mappingF));
}

} // anonymous namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QSharedPointer<MatrixXf> DISP3DRHILIB::FieldMap::computeMegMapping(
    const FwdCoilSet& coils,
    const MatrixX3f& vertices,
    const MatrixX3f& normals,
    const Vector3f& origin,
    float intrad,
    float miss)
{
    if (coils.ncoil <= 0 || vertices.rows() == 0 || normals.rows() != vertices.rows()) {
        return QSharedPointer<MatrixXf>();
    }

    const Vector3d r0 = origin.cast<double>();

    // Compute self-dot matrix (nchan × nchan) in double precision
    MatrixXd selfDots = doSelfDots(intrad, coils, r0, /*isMeg=*/true);

    // Compute surface-dot matrix (nvert × nchan) in double precision
    MatrixXd surfaceDots = doSurfaceDots(intrad, coils, vertices, normals, r0, /*isMeg=*/true);

    // Ad-hoc noise for whitening
    VectorXd stds = adHocMegStds(coils);

    return computeMappingMatrix(selfDots, surfaceDots, stds, static_cast<double>(miss));
}

QSharedPointer<MatrixXf> DISP3DRHILIB::FieldMap::computeEegMapping(
    const FwdCoilSet& coils,
    const MatrixX3f& vertices,
    const Vector3f& origin,
    float intrad,
    float miss)
{
    if (coils.ncoil <= 0 || vertices.rows() == 0) {
        return QSharedPointer<MatrixXf>();
    }

    // EEG uses scaled integration radius (matching _do_self_dots / _do_surface_dots)
    const double eegIntrad = intrad * kEegIntradScale;
    const Vector3d r0 = origin.cast<double>();

    // For EEG surface dots we still need normals; since the EEG sphere dot
    // product does not use cosmag1, we supply zero normals (they are unused).
    MatrixX3f dummyNormals = MatrixX3f::Zero(vertices.rows(), 3);

    // Compute self-dot matrix (nchan × nchan) in double precision
    MatrixXd selfDots = doSelfDots(eegIntrad, coils, r0, /*isMeg=*/false);

    // Compute surface-dot matrix (nvert × nchan) in double precision
    MatrixXd surfaceDots = doSurfaceDots(eegIntrad, coils, vertices, dummyNormals, r0, /*isMeg=*/false);

    // Ad-hoc noise for whitening
    VectorXd stds = adHocEegStds(coils.ncoil);

    return computeMappingMatrix(selfDots, surfaceDots, stds, static_cast<double>(miss));
}

