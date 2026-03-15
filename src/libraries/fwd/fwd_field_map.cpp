//=============================================================================================================
/**
 * @file     fwd_field_map.cpp
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

#include "fwd_field_map.h"

#include <fiff/fiff_proj.h>
#include <fiff/fiff_file.h>

#include <Eigen/SVD>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FWDLIB;

//=============================================================================================================
// LOCAL CONSTANTS AND HELPERS
//=============================================================================================================

namespace {

constexpr int    kNCoeff         = 100;                 // Legendre polynomial terms
constexpr double kMegConst       = 4e-14 * M_PI;        // mu_0^2 / (4*pi)
constexpr double kEegConst       = 1.0 / (4.0 * M_PI);  // 1 / (4*pi)
constexpr double kEegIntradScale = 0.7;                  // EEG integration radius scale

constexpr float  kGradStd        = 5e-13f;              // gradiometer noise std (5 fT/cm)
constexpr float  kMagStd         = 20e-15f;             // magnetometer noise std (20 fT)
constexpr float  kEegStd         = 1e-6f;               // EEG noise std (1 µV)

//=============================================================================================================

// Legendre polynomial P_n(x) with first and second derivatives for n = 0..ncoeff-1.
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

//=============================================================================================================

// Legendre polynomial P_n(x) for n = 0..ncoeff-1 (three-term recurrence).
void computeLegendreVal(double x, int ncoeff, double* p)
{
    p[0] = 1.0;
    if (ncoeff < 2) return;
    p[1] = x;
    for (int n = 2; n < ncoeff; ++n) {
        p[n] = ((2 * n - 1) * x * p[n - 1] - (n - 1) * p[n - 2]) / n;
    }
}

//=============================================================================================================

// MEG Legendre series sums (four components, n = 1..kNCoeff-1).
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

//=============================================================================================================

// EEG Legendre series sum (n = 1..kNCoeff-1).
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

//=============================================================================================================

// MEG sphere dot product for two integration points.
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

//=============================================================================================================

// EEG sphere dot product for two integration points.
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

//=============================================================================================================

// Per-coil data: normalised positions relative to sphere origin.
struct CoilData
{
    Eigen::MatrixX3d rmag;     // normalised position vectors (np × 3)
    Eigen::VectorXd  rlen;     // magnitudes (np)
    Eigen::MatrixX3d cosmag;   // direction vectors (np × 3)
    Eigen::VectorXd  w;        // integration weights (np)

    int np() const { return static_cast<int>(rlen.size()); }
};

//=============================================================================================================

// Extract and normalise coil integration-point data relative to sphere origin.
CoilData extractCoilData(const FwdCoil* coil, const Vector3d& r0)
{
    CoilData cd;
    const int n = coil->np;
    cd.rmag.resize(n, 3);
    cd.rlen.resize(n);
    cd.cosmag.resize(n, 3);
    cd.w.resize(n);

    for (int i = 0; i < n; ++i) {
        Vector3d rel = coil->rmag.row(i).cast<double>().transpose() - r0;
        double len = rel.norm();
        if (len > 0.0)
            cd.rmag.row(i) = (rel / len).transpose();
        else
            cd.rmag.row(i).setZero();
        cd.rlen(i)       = len;
        cd.cosmag.row(i) = coil->cosmag.row(i).cast<double>();
        cd.w(i)          = static_cast<double>(coil->w[i]);
    }
    return cd;
}

//=============================================================================================================

// Compute sensor self-dot-product matrix (nchan x nchan, symmetric).
MatrixXd doSelfDots(double intrad, const FwdCoilSet& coils, const Vector3d& r0, bool isMeg)
{
    const int nc = coils.ncoil();
    std::vector<CoilData> cdata(nc);
    for (int i = 0; i < nc; ++i) {
        cdata[i] = extractCoilData(coils.coils[i].get(), r0);
    }

    MatrixXd products = MatrixXd::Zero(nc, nc);
    for (int ci1 = 0; ci1 < nc; ++ci1) {
        for (int ci2 = 0; ci2 <= ci1; ++ci2) {
            double dot = 0.0;
            const CoilData& c1 = cdata[ci1];
            const CoilData& c2 = cdata[ci2];
            for (int i = 0; i < c1.np(); ++i) {
                for (int j = 0; j < c2.np(); ++j) {
                    double ww = c1.w(i) * c2.w(j);
                    if (isMeg) {
                        dot += ww * sphereDotMeg(intrad,
                                   c1.rmag.row(i).transpose(), c1.rlen(i), c1.cosmag.row(i).transpose(),
                                   c2.rmag.row(j).transpose(), c2.rlen(j), c2.cosmag.row(j).transpose());
                    } else {
                        dot += ww * sphereDotEeg(intrad,
                                   c1.rmag.row(i).transpose(), c1.rlen(i),
                                   c2.rmag.row(j).transpose(), c2.rlen(j));
                    }
                }
            }
            products(ci1, ci2) = dot;
            products(ci2, ci1) = dot;
        }
    }
    return products;
}

//=============================================================================================================

// Compute surface-to-sensor dot-product matrix (nvert x nchan).
MatrixXd doSurfaceDots(double intrad, const FwdCoilSet& coils,
                       const MatrixX3f& rr, const MatrixX3f& nn,
                       const Vector3d& r0, bool isMeg)
{
    const int nc = coils.ncoil();
    const int nv = rr.rows();

    std::vector<CoilData> cdata(nc);
    for (int i = 0; i < nc; ++i) {
        cdata[i] = extractCoilData(coils.coils[i].get(), r0);
    }

    MatrixXd products = MatrixXd::Zero(nv, nc);
    for (int vi = 0; vi < nv; ++vi) {
        // Vertex position relative to origin (normalised)
        Vector3d rel = rr.row(vi).cast<double>() - r0.transpose();
        double lsurf = rel.norm();
        Vector3d rsurf = (lsurf > 0.0) ? Vector3d(rel / lsurf) : Vector3d::Zero();
        Vector3d nsurf = nn.row(vi).cast<double>();  // surface normal (MEG cosmag)

        for (int ci = 0; ci < nc; ++ci) {
            const CoilData& c = cdata[ci];
            double dot = 0.0;
            for (int j = 0; j < c.np(); ++j) {
                if (isMeg) {
                    dot += c.w(j) * sphereDotMeg(intrad,
                               rsurf, lsurf, nsurf,
                               c.rmag.row(j).transpose(), c.rlen(j), c.cosmag.row(j).transpose());
                } else {
                    dot += c.w(j) * sphereDotEeg(intrad,
                               rsurf, lsurf,
                               c.rmag.row(j).transpose(), c.rlen(j));
                }
            }
            products(vi, ci) = dot;
        }
    }
    return products;
}

//=============================================================================================================

// MEG ad-hoc noise standard deviations.
VectorXd adHocMegStds(const FwdCoilSet& coils)
{
    VectorXd stds(coils.ncoil());
    for (int k = 0; k < coils.ncoil(); ++k) {
        stds(k) = coils.coils[k]->is_axial_coil() ? static_cast<double>(kMagStd)
                                                   : static_cast<double>(kGradStd);
    }
    return stds;
}

//=============================================================================================================

// EEG ad-hoc noise standard deviation (uniform).
VectorXd adHocEegStds(int ncoil)
{
    return VectorXd::Constant(ncoil, static_cast<double>(kEegStd));
}

//=============================================================================================================

// Compute mapping matrix via whitened SVD pseudo-inverse.
// Returns float for GPU-friendly rendering; all internal math is double.
std::unique_ptr<MatrixXf> computeMappingMatrix(const MatrixXd& selfDots,
                                              const MatrixXd& surfaceDots,
                                              const VectorXd& noiseStds,
                                              double miss,
                                              const MatrixXd& projOp = MatrixXd(),
                                              bool applyAvgRef = false)
{
    if (selfDots.rows() == 0 || surfaceDots.rows() == 0) {
        return nullptr;
    }

    const int nchan = selfDots.rows();

    // Apply SSP projector to self-dots
    MatrixXd projDots;
    bool hasProj = (projOp.rows() == nchan && projOp.cols() == nchan);
    if (hasProj) {
        projDots = projOp.transpose() * selfDots * projOp;
    } else {
        projDots = selfDots;
    }

    // Build whitener
    VectorXd whitener(nchan);
    for (int i = 0; i < nchan; ++i) {
        whitener(i) = (noiseStds(i) > 0.0) ? (1.0 / noiseStds(i)) : 0.0;
    }

    // Whiten self-dots
    MatrixXd whitenedDots = whitener.asDiagonal() * projDots * whitener.asDiagonal();

    // SVD pseudo-inverse with eigenvalue truncation
    JacobiSVD<MatrixXd> svd(whitenedDots, ComputeFullU | ComputeFullV);
    VectorXd s = svd.singularValues();
    if (s.size() == 0 || s(0) <= 0.0) {
        return nullptr;
    }

    // Eigenvalue truncation: keep components explaining >= (1-miss) of total variance
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

    // Truncated pseudo-inverse
    VectorXd sinv = VectorXd::Zero(s.size());
    for (int i = 0; i < n; ++i) {
        sinv(i) = (s(i) > 0.0) ? (1.0 / s(i)) : 0.0;
    }
    MatrixXd inv = svd.matrixV() * sinv.asDiagonal() * svd.matrixU().transpose();

    // Unwhiten inverse
    MatrixXd invWhitened = whitener.asDiagonal() * inv * whitener.asDiagonal();

    // Apply projector to inverse
    MatrixXd invWhitenedProj;
    if (hasProj) {
        invWhitenedProj = projOp.transpose() * invWhitened;
    } else {
        invWhitenedProj = invWhitened;
    }

    // Compute final mapping
    MatrixXd mapping = surfaceDots * invWhitenedProj;

    // Apply average reference for EEG
    if (applyAvgRef) {
        VectorXd colMeans = mapping.colwise().mean();
        mapping.rowwise() -= colMeans.transpose();
    }

    // Convert to float
    MatrixXf mappingF = mapping.cast<float>();
    return std::make_unique<MatrixXf>(std::move(mappingF));
}

} // anonymous namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::unique_ptr<MatrixXf> FwdFieldMap::computeMegMapping(
    const FwdCoilSet& coils,
    const MatrixX3f& vertices,
    const MatrixX3f& normals,
    const Vector3f& origin,
    float intrad,
    float miss)
{
    if (coils.ncoil() <= 0 || vertices.rows() == 0 || normals.rows() != vertices.rows()) {
        return nullptr;
    }

    const Vector3d r0 = origin.cast<double>();

    MatrixXd selfDots = doSelfDots(intrad, coils, r0, /*isMeg=*/true);
    MatrixXd surfaceDots = doSurfaceDots(intrad, coils, vertices, normals, r0, /*isMeg=*/true);
    VectorXd stds = adHocMegStds(coils);

    return computeMappingMatrix(selfDots, surfaceDots, stds, static_cast<double>(miss));
}

//=============================================================================================================

std::unique_ptr<MatrixXf> FwdFieldMap::computeMegMapping(
    const FwdCoilSet& coils,
    const MatrixX3f& vertices,
    const MatrixX3f& normals,
    const Vector3f& origin,
    const FIFFLIB::FiffInfo& info,
    const QStringList& chNames,
    float intrad,
    float miss)
{
    if (coils.ncoil() <= 0 || vertices.rows() == 0 || normals.rows() != vertices.rows()) {
        return nullptr;
    }

    const Vector3d r0 = origin.cast<double>();

    MatrixXd selfDots = doSelfDots(intrad, coils, r0, /*isMeg=*/true);
    MatrixXd surfaceDots = doSurfaceDots(intrad, coils, vertices, normals, r0, /*isMeg=*/true);
    VectorXd stds = adHocMegStds(coils);

    // Build SSP projector
    MatrixXd projOp;
    FIFFLIB::FiffProj::make_projector(info.projs, chNames, projOp, info.bads);

    return computeMappingMatrix(selfDots, surfaceDots, stds,
                                static_cast<double>(miss), projOp, false);
}

//=============================================================================================================

std::unique_ptr<MatrixXf> FwdFieldMap::computeEegMapping(
    const FwdCoilSet& coils,
    const MatrixX3f& vertices,
    const Vector3f& origin,
    float intrad,
    float miss)
{
    if (coils.ncoil() <= 0 || vertices.rows() == 0) {
        return nullptr;
    }

    const double eegIntrad = intrad * kEegIntradScale;
    const Vector3d r0 = origin.cast<double>();

    // EEG sphere dot does not use surface normals, so pass zeros
    MatrixX3f dummyNormals = MatrixX3f::Zero(vertices.rows(), 3);

    MatrixXd selfDots = doSelfDots(eegIntrad, coils, r0, /*isMeg=*/false);
    MatrixXd surfaceDots = doSurfaceDots(eegIntrad, coils, vertices, dummyNormals, r0, /*isMeg=*/false);
    VectorXd stds = adHocEegStds(coils.ncoil());

    return computeMappingMatrix(selfDots, surfaceDots, stds, static_cast<double>(miss));
}

//=============================================================================================================

std::unique_ptr<MatrixXf> FwdFieldMap::computeEegMapping(
    const FwdCoilSet& coils,
    const MatrixX3f& vertices,
    const Vector3f& origin,
    const FIFFLIB::FiffInfo& info,
    const QStringList& chNames,
    float intrad,
    float miss)
{
    if (coils.ncoil() <= 0 || vertices.rows() == 0) {
        return nullptr;
    }

    const double eegIntrad = intrad * kEegIntradScale;
    const Vector3d r0 = origin.cast<double>();

    MatrixX3f dummyNormals = MatrixX3f::Zero(vertices.rows(), 3);
    MatrixXd selfDots = doSelfDots(eegIntrad, coils, r0, /*isMeg=*/false);
    MatrixXd surfaceDots = doSurfaceDots(eegIntrad, coils, vertices, dummyNormals, r0, /*isMeg=*/false);
    VectorXd stds = adHocEegStds(coils.ncoil());

    // Build SSP projector
    MatrixXd projOp;
    FIFFLIB::FiffProj::make_projector(info.projs, chNames, projOp, info.bads);

    // Check for average EEG reference projection
    bool hasAvgRef = false;
    for (const auto& proj : info.projs) {
        if (proj.kind == FIFFV_PROJ_ITEM_EEG_AVREF ||
            proj.desc.contains(QRegularExpression("^Average .* reference$",
                                                  QRegularExpression::CaseInsensitiveOption))) {
            hasAvgRef = true;
            break;
        }
    }

    return computeMappingMatrix(selfDots, surfaceDots, stds,
                                static_cast<double>(miss), projOp, hasAvgRef);
}
