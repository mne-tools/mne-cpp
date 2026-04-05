//=============================================================================================================
/**
 * @file     inv_lcmv.cpp
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
 * @brief    Definition of the InvLCMV class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_lcmv.h"
#include "inv_beamformer_compute.h"

#include <mne/mne_forward_solution.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_info.h>
#include <math/linalg.h>

#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace INVLIB;
using namespace MNELIB;
using namespace FIFFLIB;
using namespace UTILSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InvBeamformer InvLCMV::makeLCMV(const FiffInfo &info,
                                const MNEForwardSolution &forward,
                                const FiffCov &dataCov,
                                double reg,
                                const FiffCov &noiseCov,
                                BeamformerPickOri pickOri,
                                BeamformerWeightNorm weightNorm,
                                bool reduceRank,
                                BeamformerInversion invMethod)
{
    InvBeamformer result;
    result.kind = "LCMV";

    // -----------------------------------------------------------------------
    // Extract leadfield G from forward solution
    // -----------------------------------------------------------------------
    if(!forward.sol || forward.sol->data.size() == 0) {
        qWarning("InvLCMV::makeLCMV - Forward solution has no gain matrix!");
        return result;
    }

    MatrixXd G = forward.sol->data;  // (n_channels, n_sources * n_orient)
    const int nChannels = static_cast<int>(G.rows());
    const int nOrient = (forward.source_ori == FIFFV_MNE_FREE_ORI) ? 3 : 1;
    const int nSources = static_cast<int>(G.cols()) / nOrient;

    qInfo("InvLCMV::makeLCMV - Leadfield: %d channels x %d sources (n_orient=%d)",
          nChannels, nSources, nOrient);

    // -----------------------------------------------------------------------
    // Build whitening matrix from noise covariance
    // -----------------------------------------------------------------------
    MatrixXd whitener;
    if(noiseCov.data.size() > 0) {
        // Compute whitener from noise covariance eigendecomposition
        //   whitener = diag(1/sqrt(eig)) @ eigvec^T
        if(noiseCov.eig.size() > 0 && noiseCov.eigvec.size() > 0) {
            VectorXd invSqrtEig(noiseCov.eig.size());
            for(int i = 0; i < noiseCov.eig.size(); ++i) {
                invSqrtEig(i) = (noiseCov.eig(i) > 1e-30)
                    ? 1.0 / std::sqrt(noiseCov.eig(i))
                    : 0.0;
            }
            whitener = invSqrtEig.asDiagonal() * noiseCov.eigvec.transpose();
        } else {
            // Fallback: identity whitening
            whitener = MatrixXd::Identity(nChannels, nChannels);
        }
    } else {
        whitener = MatrixXd::Identity(nChannels, nChannels);
    }

    // -----------------------------------------------------------------------
    // Build SSP projection matrix
    // -----------------------------------------------------------------------
    MatrixXd projMat = MatrixXd::Identity(nChannels, nChannels);
    // Note: SSP projections from info.projs are typically pre-applied to
    // the forward solution. If not, they should be applied here.

    // -----------------------------------------------------------------------
    // Whiten leadfield and data covariance
    //   G_w = whitener @ G
    //   Cm_w = whitener @ Cm @ whitener^T
    // -----------------------------------------------------------------------
    MatrixXd Gw = whitener * G;

    MatrixXd CmData = dataCov.data;
    if(CmData.rows() != nChannels || CmData.cols() != nChannels) {
        qWarning("InvLCMV::makeLCMV - Data covariance dimension (%d x %d) "
                 "does not match leadfield channels (%d)!",
                 static_cast<int>(CmData.rows()), static_cast<int>(CmData.cols()), nChannels);
        return result;
    }

    MatrixXd CmW = whitener * CmData * whitener.transpose();
    // Ensure Hermitian (for numerical stability)
    CmW = (CmW + CmW.transpose()) * 0.5;

    // -----------------------------------------------------------------------
    // Source normals for orientation picking
    // -----------------------------------------------------------------------
    MatrixX3d nn = forward.source_nn.cast<double>();

    // -----------------------------------------------------------------------
    // Compute spatial filter
    // -----------------------------------------------------------------------
    MatrixXd W;
    MatrixX3d mpOri;

    bool ok = InvBeamformerCompute::computeBeamformer(
        Gw, CmW, reg, nOrient,
        weightNorm, pickOri, reduceRank, invMethod,
        nn, W, mpOri);

    if(!ok) {
        qWarning("InvLCMV::makeLCMV - Beamformer computation failed!");
        return result;
    }

    // -----------------------------------------------------------------------
    // Populate result
    // -----------------------------------------------------------------------
    result.weights.push_back(W);
    result.whitener = whitener;
    result.proj = projMat;
    result.chNames = forward.sol->row_names;
    result.isFreOri = (nOrient == 3 && pickOri != BeamformerPickOri::Normal
                                    && pickOri != BeamformerPickOri::MaxPower);
    result.nSourcesTotal = nSources;
    result.srcType = "surface";
    result.weightNorm = weightNorm;
    result.pickOri = pickOri;
    result.inversion = invMethod;
    result.reg = reg;
    result.rank = static_cast<int>(CmW.rows());
    result.maxPowerOri = mpOri;
    result.sourceNn = forward.source_nn;

    // Vertex indices
    VectorXi verts(0);
    if(forward.src.size() >= 2) {
        verts.resize(forward.src[0].vertno.size() + forward.src[1].vertno.size());
        verts << forward.src[0].vertno, forward.src[1].vertno;
    } else if(forward.src.size() == 1) {
        verts = forward.src[0].vertno;
    }
    result.vertices = verts;

    qInfo("InvLCMV::makeLCMV - Done. Filter: %d x %d (sources=%d, orient=%d)",
          static_cast<int>(W.rows()), static_cast<int>(W.cols()), nSources, result.nOrient());

    return result;
}

//=============================================================================================================

MatrixXd InvLCMV::applyFilter(const MatrixXd &data, const InvBeamformer &filters)
{
    // Apply projection + whitening + spatial filter
    MatrixXd processed = data;

    // Project
    if(filters.proj.size() > 0 && filters.proj.rows() == data.rows()) {
        processed = filters.proj * processed;
    }

    // Whiten
    if(filters.whitener.size() > 0 && filters.whitener.cols() == processed.rows()) {
        processed = filters.whitener * processed;
    }

    // Apply spatial filter: sol = W @ processed
    return filters.weights[0] * processed;
}

//=============================================================================================================

InvSourceEstimate InvLCMV::applyLCMV(const FiffEvoked &evoked, const InvBeamformer &filters)
{
    if(!filters.isValid() || filters.kind != "LCMV") {
        qWarning("InvLCMV::applyLCMV - Invalid or non-LCMV filters!");
        return InvSourceEstimate();
    }

    // Pick channels from evoked to match filter channel order
    MatrixXd data;
    if(filters.chNames.size() > 0 &&
       static_cast<int>(filters.chNames.size()) != evoked.data.rows()) {
        // Need to select and reorder channels
        const int nFilterCh = static_cast<int>(filters.chNames.size());
        const int nTimes = static_cast<int>(evoked.data.cols());
        data.resize(nFilterCh, nTimes);
        for(int i = 0; i < nFilterCh; ++i) {
            int idx = evoked.info.ch_names.indexOf(filters.chNames[i]);
            if(idx < 0) {
                qWarning("InvLCMV::applyLCMV - Channel %s not found in evoked!",
                         qPrintable(filters.chNames[i]));
                return InvSourceEstimate();
            }
            data.row(i) = evoked.data.row(idx);
        }
    } else {
        data = evoked.data;
    }

    MatrixXd sol = applyFilter(data, filters);

    // Combine XYZ for free orientation if needed
    const int nOrient = filters.nOrient();
    if(nOrient == 3 && filters.pickOri != BeamformerPickOri::Vector) {
        // Combine: sqrt(x^2 + y^2 + z^2) per source per time
        const int nSources = static_cast<int>(sol.rows()) / 3;
        const int nTimes = static_cast<int>(sol.cols());
        MatrixXd combined(nSources, nTimes);
        for(int s = 0; s < nSources; ++s) {
            combined.row(s) = sol.middleRows(s * 3, 3).colwise().norm();
        }
        sol = combined;
    }

    float tmin = evoked.times.size() > 0 ? evoked.times[0] : 0.0f;
    float tstep = (evoked.info.sfreq > 0) ? 1.0f / evoked.info.sfreq : 1.0f;

    InvSourceEstimate stc(sol, filters.vertices, tmin, tstep);
    stc.method = InvEstimateMethod::LCMV;
    stc.sourceSpaceType = InvSourceSpaceType::Surface;
    stc.orientationType = filters.isFreOri ? InvOrientationType::Free : InvOrientationType::Fixed;

    return stc;
}

//=============================================================================================================

InvSourceEstimate InvLCMV::applyLCMVRaw(const MatrixXd &data,
                                        float tmin,
                                        float tstep,
                                        const InvBeamformer &filters)
{
    if(!filters.isValid() || filters.kind != "LCMV") {
        qWarning("InvLCMV::applyLCMVRaw - Invalid or non-LCMV filters!");
        return InvSourceEstimate();
    }

    MatrixXd sol = applyFilter(data, filters);

    const int nOrient = filters.nOrient();
    if(nOrient == 3 && filters.pickOri != BeamformerPickOri::Vector) {
        const int nSources = static_cast<int>(sol.rows()) / 3;
        const int nTimes = static_cast<int>(sol.cols());
        MatrixXd combined(nSources, nTimes);
        for(int s = 0; s < nSources; ++s) {
            combined.row(s) = sol.middleRows(s * 3, 3).colwise().norm();
        }
        sol = combined;
    }

    InvSourceEstimate stc(sol, filters.vertices, tmin, tstep);
    stc.method = InvEstimateMethod::LCMV;
    stc.sourceSpaceType = InvSourceSpaceType::Surface;
    stc.orientationType = filters.isFreOri ? InvOrientationType::Free : InvOrientationType::Fixed;

    return stc;
}

//=============================================================================================================

InvSourceEstimate InvLCMV::applyLCMVCov(const FiffCov &dataCov,
                                        const InvBeamformer &filters)
{
    if(!filters.isValid() || filters.kind != "LCMV") {
        qWarning("InvLCMV::applyLCMVCov - Invalid or non-LCMV filters!");
        return InvSourceEstimate();
    }

    // Whiten data covariance
    MatrixXd CmW = dataCov.data;
    if(filters.whitener.size() > 0) {
        CmW = filters.whitener * CmW * filters.whitener.transpose();
    }

    const int nOrient = filters.nOrient();
    VectorXd power = InvBeamformerCompute::computePower(CmW, filters.weights[0], nOrient);

    // Return as 1-column source estimate
    MatrixXd powerMat = power;  // (nSources, 1) implicitly via VectorXd

    InvSourceEstimate stc(powerMat, filters.vertices, 0.0f, 1.0f);
    stc.method = InvEstimateMethod::LCMV;
    stc.sourceSpaceType = InvSourceSpaceType::Surface;
    stc.orientationType = filters.isFreOri ? InvOrientationType::Free : InvOrientationType::Fixed;

    return stc;
}
