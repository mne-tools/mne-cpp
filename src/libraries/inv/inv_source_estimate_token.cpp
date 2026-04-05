//=============================================================================================================
/**
 * @file     inv_source_estimate_token.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
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
 * @brief    Tokenization and de-tokenization of InvSourceEstimate for foundation-model interfacing.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_source_estimate_token.h"
#include "inv_source_estimate.h"

#include <algorithm>
#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace Eigen;

//=============================================================================================================
// LOCAL HELPERS — enum ↔ token mapping
//=============================================================================================================

namespace {

InvTokenId methodToTokenId(InvEstimateMethod m)
{
    switch (m) {
    case InvEstimateMethod::MNE:          return InvTokenId::MethodMNE;
    case InvEstimateMethod::dSPM:         return InvTokenId::MethodDSPM;
    case InvEstimateMethod::sLORETA:      return InvTokenId::MethodSLORETA;
    case InvEstimateMethod::eLORETA:      return InvTokenId::MethodELORETA;
    case InvEstimateMethod::LCMV:         return InvTokenId::MethodLCMV;
    case InvEstimateMethod::DICS:         return InvTokenId::MethodDICS;
    case InvEstimateMethod::SAM:          return InvTokenId::MethodSAM;
    case InvEstimateMethod::MixedNorm:    return InvTokenId::MethodMixedNorm;
    case InvEstimateMethod::GammaMAP:     return InvTokenId::MethodGammaMAP;
    case InvEstimateMethod::DipoleFit:    return InvTokenId::MethodDipoleFit;
    case InvEstimateMethod::RapMusic:     return InvTokenId::MethodRapMusic;
    case InvEstimateMethod::PwlRapMusic:  return InvTokenId::MethodPwlRapMusic;
    default:                              return InvTokenId::MethodUnknown;
    }
}

InvEstimateMethod tokenIdToMethod(InvTokenId id)
{
    switch (id) {
    case InvTokenId::MethodMNE:          return InvEstimateMethod::MNE;
    case InvTokenId::MethodDSPM:         return InvEstimateMethod::dSPM;
    case InvTokenId::MethodSLORETA:      return InvEstimateMethod::sLORETA;
    case InvTokenId::MethodELORETA:      return InvEstimateMethod::eLORETA;
    case InvTokenId::MethodLCMV:         return InvEstimateMethod::LCMV;
    case InvTokenId::MethodDICS:         return InvEstimateMethod::DICS;
    case InvTokenId::MethodSAM:          return InvEstimateMethod::SAM;
    case InvTokenId::MethodMixedNorm:    return InvEstimateMethod::MixedNorm;
    case InvTokenId::MethodGammaMAP:     return InvEstimateMethod::GammaMAP;
    case InvTokenId::MethodDipoleFit:    return InvEstimateMethod::DipoleFit;
    case InvTokenId::MethodRapMusic:     return InvEstimateMethod::RapMusic;
    case InvTokenId::MethodPwlRapMusic:  return InvEstimateMethod::PwlRapMusic;
    default:                             return InvEstimateMethod::Unknown;
    }
}

InvTokenId spaceToTokenId(InvSourceSpaceType s)
{
    switch (s) {
    case InvSourceSpaceType::Surface:  return InvTokenId::SpaceSurface;
    case InvSourceSpaceType::Volume:   return InvTokenId::SpaceVolume;
    case InvSourceSpaceType::Mixed:    return InvTokenId::SpaceMixed;
    case InvSourceSpaceType::Discrete: return InvTokenId::SpaceDiscrete;
    default:                           return InvTokenId::SpaceUnknown;
    }
}

InvSourceSpaceType tokenIdToSpace(InvTokenId id)
{
    switch (id) {
    case InvTokenId::SpaceSurface:  return InvSourceSpaceType::Surface;
    case InvTokenId::SpaceVolume:   return InvSourceSpaceType::Volume;
    case InvTokenId::SpaceMixed:    return InvSourceSpaceType::Mixed;
    case InvTokenId::SpaceDiscrete: return InvSourceSpaceType::Discrete;
    default:                        return InvSourceSpaceType::Unknown;
    }
}

InvTokenId orientToTokenId(InvOrientationType o)
{
    switch (o) {
    case InvOrientationType::Fixed: return InvTokenId::OrientFixed;
    case InvOrientationType::Free:  return InvTokenId::OrientFree;
    case InvOrientationType::Loose: return InvTokenId::OrientLoose;
    default:                        return InvTokenId::OrientUnknown;
    }
}

InvOrientationType tokenIdToOrient(InvTokenId id)
{
    switch (id) {
    case InvTokenId::OrientFixed: return InvOrientationType::Fixed;
    case InvTokenId::OrientFree:  return InvOrientationType::Free;
    case InvTokenId::OrientLoose: return InvOrientationType::Loose;
    default:                      return InvOrientationType::Unknown;
    }
}

InvTokenId measureToTokenId(const std::string &m)
{
    if (m == "coh")              return InvTokenId::MeasCoh;
    if (m == "imcoh")            return InvTokenId::MeasImCoh;
    if (m == "plv")              return InvTokenId::MeasPlv;
    if (m == "pli")              return InvTokenId::MeasPli;
    if (m == "wpli")             return InvTokenId::MeasWpli;
    if (m == "granger")          return InvTokenId::MeasGranger;
    if (m == "pdc")              return InvTokenId::MeasPdc;
    if (m == "dtf")              return InvTokenId::MeasDtf;
    if (m == "correlation")      return InvTokenId::MeasCorrelation;
    if (m == "crosscorrelation") return InvTokenId::MeasCrossCorr;
    return InvTokenId::MeasOther;
}

std::string tokenIdToMeasure(InvTokenId id)
{
    switch (id) {
    case InvTokenId::MeasCoh:         return "coh";
    case InvTokenId::MeasImCoh:       return "imcoh";
    case InvTokenId::MeasPlv:         return "plv";
    case InvTokenId::MeasPli:         return "pli";
    case InvTokenId::MeasWpli:        return "wpli";
    case InvTokenId::MeasGranger:     return "granger";
    case InvTokenId::MeasPdc:         return "pdc";
    case InvTokenId::MeasDtf:         return "dtf";
    case InvTokenId::MeasCorrelation: return "correlation";
    case InvTokenId::MeasCrossCorr:   return "crosscorrelation";
    default:                          return "";
    }
}

// Check whether a token ID falls in the method label range
bool isMethodToken(InvTokenId id) {
    int v = static_cast<int>(id);
    return v >= 100 && v <= 112;
}

// Check whether a token ID falls in the source-space label range
bool isSpaceToken(InvTokenId id) {
    int v = static_cast<int>(id);
    return v >= 150 && v <= 154;
}

// Check whether a token ID falls in the orientation label range
bool isOrientToken(InvTokenId id) {
    int v = static_cast<int>(id);
    return v >= 170 && v <= 173;
}

// Check whether a token ID falls in the connectivity measure label range
bool isMeasureToken(InvTokenId id) {
    int v = static_cast<int>(id);
    return v >= 300 && v <= 310;
}

// Compute sub-sampling stride
int stride(int total, int max)
{
    if (max <= 0 || max >= total) return 1;
    return std::max(1, total / max);
}

} // anonymous namespace

namespace INVLIB
{

//=============================================================================================================
// TOKENIZE
//=============================================================================================================

std::vector<InvToken> tokenize(const InvSourceEstimate &estimate, const InvTokenizeOptions &options)
{
    std::vector<InvToken> tokens;

    // Rough capacity estimate to avoid excessive reallocations
    int nSrc   = static_cast<int>(estimate.data.rows());
    int nTimes = static_cast<int>(estimate.data.cols());
    int srcStride  = stride(nSrc,   options.maxSources);
    int timeStride = stride(nTimes, options.maxTimePoints);
    int effSrc  = (nSrc  + srcStride  - 1) / srcStride;
    int effTime = (nTimes + timeStride - 1) / timeStride;

    size_t est = 10;  // structural overhead
    if (options.includeGridData && estimate.hasGridData())
        est += static_cast<size_t>(effSrc) * (1 + effTime) + effSrc + 6;
    est += estimate.focalDipoles.size() * 16;
    est += estimate.couplings.size() * 20;
    est += estimate.connectivity.size() * 10;
    if (estimate.hasPositions())
        est += static_cast<size_t>(estimate.positions.rows()) * 3 + 2;
    tokens.reserve(est);

    // --- BOS ---
    tokens.emplace_back(InvTokenId::Bos);

    // --- Metadata ---
    tokens.emplace_back(InvTokenId::MetaBegin);
    tokens.emplace_back(methodToTokenId(estimate.method));
    tokens.emplace_back(spaceToTokenId(estimate.sourceSpaceType));
    tokens.emplace_back(orientToTokenId(estimate.orientationType));
    tokens.emplace_back(InvTokenId::MetaEnd);

    // --- Grid data ---
    if (options.includeGridData && estimate.hasGridData()) {
        tokens.emplace_back(InvTokenId::GridBegin);
        tokens.emplace_back(InvTokenId::NSources, static_cast<float>(effSrc));
        tokens.emplace_back(InvTokenId::NTimes,   static_cast<float>(effTime));
        tokens.emplace_back(InvTokenId::TimeVal,   estimate.tmin);
        tokens.emplace_back(InvTokenId::TStep,     estimate.tstep);

        // Vertex indices
        for (int s = 0; s < nSrc; s += srcStride)
            tokens.emplace_back(InvTokenId::Vertex, static_cast<float>(estimate.vertices[s]));

        // Amplitude data row by row
        for (int s = 0; s < nSrc; s += srcStride) {
            tokens.emplace_back(InvTokenId::GridRow);
            for (int t = 0; t < nTimes; t += timeStride)
                tokens.emplace_back(InvTokenId::Amplitude, static_cast<float>(estimate.data(s, t)));
        }
        tokens.emplace_back(InvTokenId::GridEnd);
    }

    // --- Positions ---
    if (options.includePositions && estimate.hasPositions()) {
        tokens.emplace_back(InvTokenId::PosBegin);
        int nPos = static_cast<int>(estimate.positions.rows());
        for (int i = 0; i < nPos; i += srcStride) {
            tokens.emplace_back(InvTokenId::PosX, estimate.positions(i, 0));
            tokens.emplace_back(InvTokenId::PosY, estimate.positions(i, 1));
            tokens.emplace_back(InvTokenId::PosZ, estimate.positions(i, 2));
        }
        tokens.emplace_back(InvTokenId::PosEnd);
    }

    // --- Couplings ---
    if (options.includeCouplings && estimate.hasCouplings()) {
        tokens.emplace_back(InvTokenId::CouplingBegin);
        tokens.emplace_back(InvTokenId::NGroups, static_cast<float>(estimate.couplings.size()));

        for (const auto &grp : estimate.couplings) {
            tokens.emplace_back(InvTokenId::GroupBegin);
            tokens.emplace_back(InvTokenId::TimeVal, grp.tmin);
            tokens.emplace_back(InvTokenId::TimeVal, grp.tmax);
            tokens.emplace_back(InvTokenId::NIndices, static_cast<float>(grp.gridIndices.size()));

            for (size_t k = 0; k < grp.gridIndices.size(); ++k) {
                tokens.emplace_back(InvTokenId::GridIndex, static_cast<float>(grp.gridIndices[k]));
                if (k < grp.moments.size()) {
                    tokens.emplace_back(InvTokenId::MomX, static_cast<float>(grp.moments[k].x()));
                    tokens.emplace_back(InvTokenId::MomY, static_cast<float>(grp.moments[k].y()));
                    tokens.emplace_back(InvTokenId::MomZ, static_cast<float>(grp.moments[k].z()));
                }
            }

            // Upper-triangle of N×N correlation matrix
            int n = static_cast<int>(grp.gridIndices.size());
            for (int r = 0; r < n && r < grp.correlations.rows(); ++r)
                for (int c = r; c < n && c < grp.correlations.cols(); ++c)
                    tokens.emplace_back(InvTokenId::Correlation, static_cast<float>(grp.correlations(r, c)));

            tokens.emplace_back(InvTokenId::GroupEnd);
        }
        tokens.emplace_back(InvTokenId::CouplingEnd);
    }

    // --- Focal dipoles ---
    if (options.includeFocalDipoles && estimate.hasFocalDipoles()) {
        tokens.emplace_back(InvTokenId::FocalBegin);
        tokens.emplace_back(InvTokenId::NDipoles, static_cast<float>(estimate.focalDipoles.size()));

        for (const auto &dip : estimate.focalDipoles) {
            tokens.emplace_back(InvTokenId::DipoleBegin);
            tokens.emplace_back(InvTokenId::TimeVal, dip.tmin);
            tokens.emplace_back(InvTokenId::TimeVal, dip.tmax);
            tokens.emplace_back(InvTokenId::PosX, dip.position.x());
            tokens.emplace_back(InvTokenId::PosY, dip.position.y());
            tokens.emplace_back(InvTokenId::PosZ, dip.position.z());
            tokens.emplace_back(InvTokenId::MomX, dip.moment.x());
            tokens.emplace_back(InvTokenId::MomY, dip.moment.y());
            tokens.emplace_back(InvTokenId::MomZ, dip.moment.z());
            tokens.emplace_back(InvTokenId::GridIndex, static_cast<float>(dip.gridIndex));
            tokens.emplace_back(InvTokenId::Goodness, dip.goodness);
            tokens.emplace_back(InvTokenId::ChiSquared, dip.khi2);
            tokens.emplace_back(InvTokenId::NFreeDof, static_cast<float>(dip.nfree));
            tokens.emplace_back(dip.valid ? InvTokenId::ValidTrue : InvTokenId::ValidFalse);
            tokens.emplace_back(InvTokenId::DipoleEnd);
        }
        tokens.emplace_back(InvTokenId::FocalEnd);
    }

    // --- Connectivity ---
    if (options.includeConnectivity && estimate.hasConnectivity()) {
        tokens.emplace_back(InvTokenId::ConnBegin);
        tokens.emplace_back(InvTokenId::NMeasures, static_cast<float>(estimate.connectivity.size()));

        for (const auto &conn : estimate.connectivity) {
            tokens.emplace_back(InvTokenId::ConnEntryBegin);
            tokens.emplace_back(measureToTokenId(conn.measure));
            tokens.emplace_back(conn.directed ? InvTokenId::DirectedTrue : InvTokenId::DirectedFalse);
            tokens.emplace_back(InvTokenId::FreqVal, conn.fmin);
            tokens.emplace_back(InvTokenId::FreqVal, conn.fmax);
            tokens.emplace_back(InvTokenId::TimeVal, conn.tmin);
            tokens.emplace_back(InvTokenId::TimeVal, conn.tmax);

            int n = static_cast<int>(conn.matrix.rows());
            tokens.emplace_back(InvTokenId::NSources, static_cast<float>(n));
            for (int r = 0; r < n; ++r)
                for (int c = 0; c < n; ++c)
                    tokens.emplace_back(InvTokenId::ConnValue, static_cast<float>(conn.matrix(r, c)));

            tokens.emplace_back(InvTokenId::ConnEntryEnd);
        }
        tokens.emplace_back(InvTokenId::ConnEnd);
    }

    // --- EOS ---
    tokens.emplace_back(InvTokenId::Eos);
    return tokens;
}

//=============================================================================================================
// FROM TOKENS — reconstruct InvSourceEstimate from a token sequence
//=============================================================================================================

InvSourceEstimate fromTokens(const std::vector<InvToken> &tokens)
{
    InvSourceEstimate est;
    size_t pos = 0;
    const size_t len = tokens.size();

    auto advance = [&]() -> const InvToken& {
        return tokens[pos++];
    };

    auto peek = [&]() -> InvTokenId {
        return (pos < len) ? tokens[pos].id : InvTokenId::Eos;
    };

    // Skip BOS
    if (pos < len && tokens[pos].id == InvTokenId::Bos) ++pos;

    while (pos < len && tokens[pos].id != InvTokenId::Eos) {
        const InvToken &tok = tokens[pos];

        // --- Metadata ---
        if (tok.id == InvTokenId::MetaBegin) {
            ++pos;
            while (pos < len && peek() != InvTokenId::MetaEnd) {
                if (isMethodToken(peek()))
                    est.method = tokenIdToMethod(advance().id);
                else if (isSpaceToken(peek()))
                    est.sourceSpaceType = tokenIdToSpace(advance().id);
                else if (isOrientToken(peek()))
                    est.orientationType = tokenIdToOrient(advance().id);
                else
                    ++pos;
            }
            if (pos < len) ++pos; // skip MetaEnd
        }

        // --- Grid data ---
        else if (tok.id == InvTokenId::GridBegin) {
            ++pos;
            int nSrc = 0, nTime = 0;
            std::vector<int> verts;
            std::vector<std::vector<float>> rows;

            while (pos < len && peek() != InvTokenId::GridEnd) {
                const InvToken &g = tokens[pos];

                if (g.id == InvTokenId::NSources) {
                    nSrc = static_cast<int>(g.value); ++pos;
                } else if (g.id == InvTokenId::NTimes) {
                    nTime = static_cast<int>(g.value); ++pos;
                } else if (g.id == InvTokenId::TimeVal && est.tmin == 0 && est.tstep == -1) {
                    est.tmin = g.value; ++pos;
                } else if (g.id == InvTokenId::TStep) {
                    est.tstep = g.value; ++pos;
                } else if (g.id == InvTokenId::TimeVal) {
                    // Second TimeVal inside grid is still tmin if tstep wasn't set yet
                    ++pos;
                } else if (g.id == InvTokenId::Vertex) {
                    verts.push_back(static_cast<int>(g.value)); ++pos;
                } else if (g.id == InvTokenId::GridRow) {
                    ++pos;
                    std::vector<float> row;
                    while (pos < len && tokens[pos].id == InvTokenId::Amplitude) {
                        row.push_back(tokens[pos].value);
                        ++pos;
                    }
                    rows.push_back(std::move(row));
                } else {
                    ++pos;
                }
            }
            if (pos < len) ++pos; // skip GridEnd

            // Fill Eigen structures
            int effSrc  = static_cast<int>(rows.size());
            int effTime = effSrc > 0 ? static_cast<int>(rows[0].size()) : 0;
            if (effSrc > 0 && effTime > 0) {
                est.data = MatrixXd(effSrc, effTime);
                for (int s = 0; s < effSrc; ++s)
                    for (int t = 0; t < effTime && t < static_cast<int>(rows[s].size()); ++t)
                        est.data(s, t) = static_cast<double>(rows[s][t]);
            }
            est.vertices = VectorXi(static_cast<int>(verts.size()));
            for (int i = 0; i < static_cast<int>(verts.size()); ++i)
                est.vertices[i] = verts[i];
        }

        // --- Positions ---
        else if (tok.id == InvTokenId::PosBegin) {
            ++pos;
            std::vector<Vector3f> posVec;
            while (pos + 2 < len && peek() != InvTokenId::PosEnd) {
                if (tokens[pos].id == InvTokenId::PosX) {
                    Vector3f p;
                    p.x() = tokens[pos].value;
                    p.y() = tokens[pos + 1].value;
                    p.z() = tokens[pos + 2].value;
                    posVec.push_back(p);
                    pos += 3;
                } else {
                    ++pos;
                }
            }
            if (pos < len) ++pos; // skip PosEnd

            est.positions = MatrixX3f(static_cast<int>(posVec.size()), 3);
            for (int i = 0; i < static_cast<int>(posVec.size()); ++i)
                est.positions.row(i) = posVec[i].transpose();
        }

        // --- Couplings ---
        else if (tok.id == InvTokenId::CouplingBegin) {
            ++pos;
            if (pos < len && tokens[pos].id == InvTokenId::NGroups) ++pos; // skip count

            while (pos < len && peek() != InvTokenId::CouplingEnd) {
                if (peek() == InvTokenId::GroupBegin) {
                    ++pos;
                    InvSourceCoupling grp;
                    int nIdx = 0;

                    while (pos < len && peek() != InvTokenId::GroupEnd) {
                        const InvToken &ct = tokens[pos];
                        if (ct.id == InvTokenId::TimeVal) {
                            if (grp.tmin == 0.0f && grp.tmax == 0.0f)
                                grp.tmin = ct.value;
                            else
                                grp.tmax = ct.value;
                            ++pos;
                        } else if (ct.id == InvTokenId::NIndices) {
                            nIdx = static_cast<int>(ct.value); ++pos;
                        } else if (ct.id == InvTokenId::GridIndex) {
                            grp.gridIndices.push_back(static_cast<int>(ct.value)); ++pos;
                        } else if (ct.id == InvTokenId::MomX && pos + 2 < len) {
                            Vector3d m;
                            m.x() = static_cast<double>(tokens[pos].value);
                            m.y() = static_cast<double>(tokens[pos + 1].value);
                            m.z() = static_cast<double>(tokens[pos + 2].value);
                            grp.moments.push_back(m);
                            pos += 3;
                        } else if (ct.id == InvTokenId::Correlation) {
                            // Reconstruct upper-triangle
                            int n = static_cast<int>(grp.gridIndices.size());
                            if (n > 0 && grp.correlations.size() == 0) {
                                grp.correlations = MatrixXd::Zero(n, n);
                                for (int r = 0; r < n; ++r) {
                                    for (int c = r; c < n && pos < len && tokens[pos].id == InvTokenId::Correlation; ++c) {
                                        double v = static_cast<double>(tokens[pos].value);
                                        grp.correlations(r, c) = v;
                                        grp.correlations(c, r) = v;
                                        ++pos;
                                    }
                                }
                            } else {
                                ++pos;
                            }
                        } else {
                            ++pos;
                        }
                    }
                    if (pos < len) ++pos; // skip GroupEnd
                    est.couplings.push_back(std::move(grp));
                } else {
                    ++pos;
                }
            }
            if (pos < len) ++pos; // skip CouplingEnd
        }

        // --- Focal dipoles ---
        else if (tok.id == InvTokenId::FocalBegin) {
            ++pos;
            if (pos < len && tokens[pos].id == InvTokenId::NDipoles) ++pos;

            while (pos < len && peek() != InvTokenId::FocalEnd) {
                if (peek() == InvTokenId::DipoleBegin) {
                    ++pos;
                    InvFocalDipole dip;
                    int timeIdx = 0;

                    while (pos < len && peek() != InvTokenId::DipoleEnd) {
                        const InvToken &dt = tokens[pos];
                        if (dt.id == InvTokenId::TimeVal) {
                            if (timeIdx == 0) dip.tmin = dt.value;
                            else              dip.tmax = dt.value;
                            ++timeIdx; ++pos;
                        } else if (dt.id == InvTokenId::PosX) { dip.position.x() = dt.value; ++pos; }
                        else if (dt.id == InvTokenId::PosY) { dip.position.y() = dt.value; ++pos; }
                        else if (dt.id == InvTokenId::PosZ) { dip.position.z() = dt.value; ++pos; }
                        else if (dt.id == InvTokenId::MomX) { dip.moment.x() = dt.value; ++pos; }
                        else if (dt.id == InvTokenId::MomY) { dip.moment.y() = dt.value; ++pos; }
                        else if (dt.id == InvTokenId::MomZ) { dip.moment.z() = dt.value; ++pos; }
                        else if (dt.id == InvTokenId::GridIndex) { dip.gridIndex = static_cast<int>(dt.value); ++pos; }
                        else if (dt.id == InvTokenId::Goodness)  { dip.goodness = dt.value; ++pos; }
                        else if (dt.id == InvTokenId::ChiSquared){ dip.khi2 = dt.value; ++pos; }
                        else if (dt.id == InvTokenId::NFreeDof)  { dip.nfree = static_cast<int>(dt.value); ++pos; }
                        else if (dt.id == InvTokenId::ValidTrue) { dip.valid = true; ++pos; }
                        else if (dt.id == InvTokenId::ValidFalse){ dip.valid = false; ++pos; }
                        else { ++pos; }
                    }
                    if (pos < len) ++pos; // skip DipoleEnd
                    est.focalDipoles.push_back(dip);
                } else {
                    ++pos;
                }
            }
            if (pos < len) ++pos; // skip FocalEnd
        }

        // --- Connectivity ---
        else if (tok.id == InvTokenId::ConnBegin) {
            ++pos;
            if (pos < len && tokens[pos].id == InvTokenId::NMeasures) ++pos;

            while (pos < len && peek() != InvTokenId::ConnEnd) {
                if (peek() == InvTokenId::ConnEntryBegin) {
                    ++pos;
                    InvConnectivity conn;
                    int n = 0;
                    int freqIdx = 0, timeIdx = 0;

                    while (pos < len && peek() != InvTokenId::ConnEntryEnd) {
                        const InvToken &ct = tokens[pos];
                        if (isMeasureToken(ct.id)) {
                            conn.measure = tokenIdToMeasure(ct.id); ++pos;
                        } else if (ct.id == InvTokenId::DirectedTrue) { conn.directed = true; ++pos; }
                        else if (ct.id == InvTokenId::DirectedFalse)  { conn.directed = false; ++pos; }
                        else if (ct.id == InvTokenId::FreqVal) {
                            if (freqIdx == 0) conn.fmin = ct.value;
                            else              conn.fmax = ct.value;
                            ++freqIdx; ++pos;
                        } else if (ct.id == InvTokenId::TimeVal) {
                            if (timeIdx == 0) conn.tmin = ct.value;
                            else              conn.tmax = ct.value;
                            ++timeIdx; ++pos;
                        } else if (ct.id == InvTokenId::NSources) {
                            n = static_cast<int>(ct.value); ++pos;
                        } else if (ct.id == InvTokenId::ConnValue && n > 0) {
                            conn.matrix = MatrixXd(n, n);
                            for (int r = 0; r < n; ++r)
                                for (int c = 0; c < n && pos < len && tokens[pos].id == InvTokenId::ConnValue; ++c, ++pos)
                                    conn.matrix(r, c) = static_cast<double>(tokens[pos].value);
                        } else {
                            ++pos;
                        }
                    }
                    if (pos < len) ++pos; // skip ConnEntryEnd
                    est.connectivity.push_back(std::move(conn));
                } else {
                    ++pos;
                }
            }
            if (pos < len) ++pos; // skip ConnEnd
        }

        else {
            ++pos; // skip unrecognised tokens
        }
    }

    // Rebuild times vector
    if (est.data.cols() > 0 && est.tstep > 0) {
        est.times = RowVectorXf(est.data.cols());
        est.times[0] = est.tmin;
        for (int i = 1; i < est.times.size(); ++i)
            est.times[i] = est.times[i - 1] + est.tstep;
    }

    return est;
}

} // namespace INVLIB
