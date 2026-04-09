//=============================================================================================================
/**
 * @file     inv_dics.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    2.1.0
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
 * @brief    Definition of the InvDICS class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_dics.h"
#include "inv_beamformer_compute.h"

#include <mne/mne_forward_solution.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>

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

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InvBeamformer InvDICS::makeDICS(const FiffInfo &info,
                                const MNEForwardSolution &forward,
                                const std::vector<MatrixXd> &csdMatrices,
                                const VectorXd &frequencies,
                                double reg,
                                bool realFilter,
                                const FiffCov &noiseCov,
                                BeamformerPickOri pickOri,
                                BeamformerWeightNorm weightNorm,
                                bool reduceRank,
                                BeamformerInversion invMethod)
{
    InvBeamformer result;
    result.kind = "DICS";

    const int nFreqs = static_cast<int>(csdMatrices.size());
    if(nFreqs == 0) {
        qWarning("InvDICS::makeDICS - No CSD matrices provided!");
        return result;
    }
    if(frequencies.size() != nFreqs) {
        qWarning("InvDICS::makeDICS - Frequency vector size mismatch with CSD count!");
        return result;
    }

    // -----------------------------------------------------------------------
    // Extract leadfield
    // -----------------------------------------------------------------------
    if(!forward.sol || forward.sol->data.size() == 0) {
        qWarning("InvDICS::makeDICS - Forward solution has no gain matrix!");
        return result;
    }

    MatrixXd G = forward.sol->data;
    const int nChannels = static_cast<int>(G.rows());
    const int nOrient = (forward.source_ori == FIFFV_MNE_FREE_ORI) ? 3 : 1;
    const int nSources = static_cast<int>(G.cols()) / nOrient;

    qInfo("InvDICS::makeDICS - Leadfield: %d channels x %d sources (n_orient=%d), %d frequencies",
          nChannels, nSources, nOrient, nFreqs);

    // -----------------------------------------------------------------------
    // Whitening matrix
    // -----------------------------------------------------------------------
    MatrixXd whitener;
    if(noiseCov.data.size() > 0 && noiseCov.eig.size() > 0 && noiseCov.eigvec.size() > 0) {
        VectorXd invSqrtEig(noiseCov.eig.size());
        for(int i = 0; i < noiseCov.eig.size(); ++i) {
            invSqrtEig(i) = (noiseCov.eig(i) > 1e-30)
                ? 1.0 / std::sqrt(noiseCov.eig(i))
                : 0.0;
        }
        whitener = invSqrtEig.asDiagonal() * noiseCov.eigvec.transpose();
    } else {
        whitener = MatrixXd::Identity(nChannels, nChannels);
    }

    MatrixXd projMat = MatrixXd::Identity(nChannels, nChannels);

    // Whiten leadfield (shared across all frequencies)
    MatrixXd Gw = whitener * G;

    // Source normals
    MatrixX3d nn = forward.source_nn.cast<double>();

    // -----------------------------------------------------------------------
    // Compute filter for each frequency
    // -----------------------------------------------------------------------
    for(int fi = 0; fi < nFreqs; ++fi) {
        MatrixXd Cm = csdMatrices[fi];

        if(Cm.rows() != nChannels || Cm.cols() != nChannels) {
            qWarning("InvDICS::makeDICS - CSD[%d] dimension mismatch!", fi);
            return InvBeamformer();
        }

        // Optional: take real part of CSD
        if(realFilter) {
            // CSD is provided as real-valued after user extracts real part,
            // or we ensure it here
            Cm = Cm.real();
        }

        // Whiten CSD
        MatrixXd CmW = whitener * Cm * whitener.transpose();
        CmW = (CmW + CmW.transpose()) * 0.5;  // Ensure symmetry

        // Compute filter
        MatrixXd W;
        MatrixX3d mpOri;

        bool ok = InvBeamformerCompute::computeBeamformer(
            Gw, CmW, reg, nOrient,
            weightNorm, pickOri, reduceRank, invMethod,
            nn, W, mpOri);

        if(!ok) {
            qWarning("InvDICS::makeDICS - Filter computation failed at frequency %d (%.1f Hz)!",
                     fi, frequencies(fi));
            return InvBeamformer();
        }

        result.weights.push_back(W);

        // Store max-power orientation from first frequency
        if(fi == 0 && pickOri == BeamformerPickOri::MaxPower) {
            result.maxPowerOri = mpOri;
        }
    }

    // -----------------------------------------------------------------------
    // Populate metadata
    // -----------------------------------------------------------------------
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
    result.rank = static_cast<int>(nChannels);
    result.sourceNn = forward.source_nn;
    result.frequencies = frequencies;

    VectorXi verts(0);
    if(forward.src.size() >= 2) {
        verts.resize(forward.src[0].vertno.size() + forward.src[1].vertno.size());
        verts << forward.src[0].vertno, forward.src[1].vertno;
    } else if(forward.src.size() == 1) {
        verts = forward.src[0].vertno;
    }
    result.vertices = verts;

    qInfo("InvDICS::makeDICS - Done. %d frequency filters computed.", nFreqs);

    return result;
}

//=============================================================================================================

InvSourceEstimate InvDICS::applyDICSCsd(const std::vector<MatrixXd> &csdMatrices,
                                        const VectorXd &frequencies,
                                        const InvBeamformer &filters)
{
    if(!filters.isValid() || filters.kind != "DICS") {
        qWarning("InvDICS::applyDICSCsd - Invalid or non-DICS filters!");
        return InvSourceEstimate();
    }

    const int nFreqs = static_cast<int>(csdMatrices.size());
    const int nFilterFreqs = filters.nFreqs();

    if(nFreqs != nFilterFreqs) {
        qWarning("InvDICS::applyDICSCsd - CSD count (%d) does not match filter count (%d)!",
                 nFreqs, nFilterFreqs);
        return InvSourceEstimate();
    }

    const int nOrient = filters.nOrient();
    const int nSources = filters.nSources();
    const int nChannels = filters.nChannels();

    // Power matrix: (nSources, nFreqs)
    MatrixXd powerMat(nSources, nFreqs);

    for(int fi = 0; fi < nFreqs; ++fi) {
        MatrixXd Cm = csdMatrices[fi];

        // Whiten CSD
        if(filters.whitener.size() > 0) {
            Cm = filters.whitener * Cm * filters.whitener.transpose();
        }

        VectorXd power = InvBeamformerCompute::computePower(Cm, filters.weights[fi], nOrient);
        powerMat.col(fi) = power;
    }

    // Use frequency as "time" axis for the source estimate
    float fmin = (frequencies.size() > 0) ? static_cast<float>(frequencies(0)) : 0.0f;
    float fstep = (frequencies.size() > 1)
        ? static_cast<float>(frequencies(1) - frequencies(0))
        : 1.0f;

    InvSourceEstimate stc(powerMat, filters.vertices, fmin, fstep);
    stc.method = InvEstimateMethod::DICS;
    stc.sourceSpaceType = InvSourceSpaceType::Surface;
    stc.orientationType = filters.isFreOri ? InvOrientationType::Free : InvOrientationType::Fixed;

    return stc;
}

//=============================================================================================================

InvSourceEstimate InvDICS::applyDICS(const MatrixXd &data,
                                     float tmin,
                                     float tstep,
                                     const InvBeamformer &filters,
                                     int freqIdx)
{
    if(!filters.isValid() || filters.kind != "DICS") {
        qWarning("InvDICS::applyDICS - Invalid or non-DICS filters!");
        return InvSourceEstimate();
    }
    if(freqIdx < 0 || freqIdx >= filters.nFreqs()) {
        qWarning("InvDICS::applyDICS - freqIdx %d out of range (0..%d)!",
                 freqIdx, filters.nFreqs() - 1);
        return InvSourceEstimate();
    }

    // Apply projection + whitening + spatial filter
    MatrixXd processed = data;
    if(filters.proj.size() > 0 && filters.proj.rows() == data.rows()) {
        processed = filters.proj * processed;
    }
    if(filters.whitener.size() > 0 && filters.whitener.rows() == processed.rows()) {
        processed = filters.whitener * processed;
    }

    MatrixXd sol = filters.weights[freqIdx] * processed;

    // Combine XYZ if needed
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
    stc.method = InvEstimateMethod::DICS;
    stc.sourceSpaceType = InvSourceSpaceType::Surface;
    stc.orientationType = filters.isFreOri ? InvOrientationType::Free : InvOrientationType::Fixed;

    return stc;
}
