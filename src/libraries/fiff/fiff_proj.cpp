//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_proj.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Florian Schlembach <fschlembach@web.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     October 2012
 * @brief    Implementation of @ref FiffProj: read / write of one FIFFB_PROJ_ITEM block and the projector-combination helpers.
 *
 * Multiple @ref FiffProj instances form @c info['projs'] and are
 * combined into one projection operator by @c make_projector at raw /
 * evoked load time.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_proj.h"
#include "fiff_ch_name.h"
#include <base/log.h>

#include <format>
#include <map>
#include <string>

#include <algorithm>
#include <iterator>

#include <vector>

#include <set>
#include <base/string_utils.h>
#include "fiff_raw_data.h"
#include "fiff_evoked.h"
#include "fiff_constants.h"
#include "fiff_file.h"
#include <stdio.h>
#include <cmath>
#include <math/linalg.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/SVD>

#include <array>
#include <stdexcept>
#include <cstdint>
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE STATIC FUNCTIONS
//=============================================================================================================

namespace
{

/** Good gradiometer, magnetometer and EEG channel indices, in that order (mne "grad", "mag", "eeg"). */
std::array<std::vector<int>, 3> pcaChannelGroups(const FiffInfo& info)
{
    std::array<std::vector<int>, 3> groups;
    for (int k = 0; k < std::ssize(info.chs); ++k) {
        if (has_channel(info.bads, info.ch_names[k])) {
            continue;
        }
        if (info.chs[k].kind == FIFFV_MEG_CH) {
            groups[info.chs[k].unit == FIFF_UNIT_T ? 1 : 0].push_back(k);
        } else if (info.chs[k].kind == FIFFV_EEG_CH) {
            groups[2].push_back(k);
        }
    }
    return groups;
}

/**
 * Unprojected PCA projectors of data (channels x samples) restricted to chIdx: the first nVec left
 * singular vectors, i.e. eigenvectors of the uncentred second moment, padded to all channels.
 */
std::vector<FiffProj> pcaProjectors(const MatrixXd& data, const std::vector<int>& chIdx, const StringList& chNames,
                                    int nVec, const StringList& descs)
{
    std::vector<FiffProj> projs;
    if (nVec <= 0 || chIdx.empty()) {
        return projs;
    }
    MatrixXd sub(chIdx.size(), data.cols());
    for (int c = 0; c < std::ssize(chIdx); ++c) {
        sub.row(c) = data.row(chIdx[c]);
    }
    const JacobiSVD<MatrixXd> svd(sub, ComputeThinU);
    const int nComp = std::min(nVec, static_cast<int>(svd.matrixU().cols()));
    for (int v = 0; v < nComp; ++v) {
        FiffProj proj;
        proj.kind = FIFFV_PROJ_ITEM_FIELD;
        proj.active = false;
        proj.desc = descs[static_cast<std::size_t>(v)];
        proj.data.nrow = 1;
        proj.data.ncol = static_cast<int>(chNames.size());
        proj.data.col_names = chNames;
        proj.data.data = MatrixXd::Zero(1, chNames.size());
        for (int c = 0; c < std::ssize(chIdx); ++c) {
            proj.data.data(0, chIdx[c]) = svd.matrixU()(c, v);
        }
        projs.push_back(proj);
    }
    return projs;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffProj::FiffProj()
: kind(-1)
, active(false)
, desc("")
{
}

//=============================================================================================================

FiffProj::FiffProj(const FiffProj& p_FiffProj)
: kind(p_FiffProj.kind)
, active(p_FiffProj.active)
, desc(p_FiffProj.desc)
, data(p_FiffProj.data)
{
}

//=============================================================================================================

FiffProj::FiffProj(fiff_int_t p_kind, bool p_active, std::string p_desc, FiffNamedMatrix& p_data)
: kind(p_kind)
, active(p_active)
, desc(std::move(p_desc))
, data(p_data)
{
}

//=============================================================================================================

FiffProj::~FiffProj()
{
}

//=============================================================================================================

void FiffProj::activate_projs(std::vector<FiffProj>& p_qListFiffProj)
{
    // Activate the projection items
    for (FiffProj& proj : p_qListFiffProj)
        proj.active = true;

    Log::info("\t{} projection items activated.\n", p_qListFiffProj.size());
}

//=============================================================================================================

fiff_int_t FiffProj::make_projector(const std::vector<FiffProj>& projs, const StringList& ch_names, MatrixXd& proj, const StringList& bads, MatrixXd& U)
{
    fiff_int_t nchan = static_cast<int>(ch_names.size());
    if (nchan == 0) {
        throw std::invalid_argument("No channel names specified");
    }

    //    if(proj)
    //        delete proj;
    proj = MatrixXd::Identity(nchan, nchan);
    fiff_int_t nproj = 0;
    U = MatrixXd();

    //
    //   Check trivial cases first
    //
    if (projs.size() == 0)
        return 0;

    fiff_int_t nvec = 0;
    fiff_int_t k;
    for (k = 0; k < std::ssize(projs); ++k) {
        if (projs[k].active) {
            ++nproj;
            nvec += projs[k].data.nrow;
        }
    }

    if (nproj == 0) {
        Log::warning("FiffProj::make_projector - No projectors nproj=0\n");
        return 0;
    }

    if (nvec <= 0) {
        Log::warning("FiffProj::make_projector - No rows in projector matrices found nvec<=0\n");
        return 0;
    }

    //
    //   Pick the appropriate entries
    //
    MatrixXd vecs = MatrixXd::Zero(nchan, nvec);
    nvec = 0;
    fiff_int_t nonzero = 0;
    std::int32_t p, c, i, j, v;
    double onesize;
    bool isBad = false;
    RowVectorXi sel(nchan);
    RowVectorXi vecSel(nchan);
    sel.setConstant(-1);
    vecSel.setConstant(-1);
    for (k = 0; k < std::ssize(projs); ++k) {
        if (projs[k].active) {
            FiffProj one = projs[k];

            const std::set<std::string> unique(one.data.col_names.begin(), one.data.col_names.end());
            if (one.data.col_names.size() != unique.size()) {
                Log::warning("Channel name list in projection item {} contains duplicate items", k);
                return 0;
            }

            //
            // Get the two selection vectors to pick correct elements from
            // the projection vectors omitting bad channels
            //
            sel.resize(nchan);
            vecSel.resize(nchan);
            sel.setConstant(-1);
            vecSel.setConstant(-1);
            p = 0;
            for (c = 0; c < nchan; ++c) {
                for (i = 0; i < static_cast<int>(one.data.col_names.size()); ++i) {
                    if (same_channel(ch_names[c], one.data.col_names[i])) {
                        isBad = false;
                        for (j = 0; j < static_cast<int>(bads.size()); ++j) {
                            if (same_channel(ch_names[c], bads[j])) {
                                isBad = true;
                            }
                        }

                        if (!isBad && sel[p] != c) {
                            sel[p] = c;
                            vecSel[p] = i;
                            ++p;
                        }
                    }
                }
            }
            sel.conservativeResize(p);
            vecSel.conservativeResize(p);
            //
            // If there is something to pick, pickit
            //
            if (sel.cols() > 0)
                for (v = 0; v < one.data.nrow; ++v)
                    for (i = 0; i < p; ++i)
                        vecs(sel[i], nvec + v) = one.data.data(v, vecSel[i]);

            //
            //   Rescale for more straightforward detection of small singular values
            //
            for (v = 0; v < one.data.nrow; ++v) {
                onesize = sqrt((vecs.col(nvec + v).transpose() * vecs.col(nvec + v))(0, 0));
                if (onesize > 0.0) {
                    vecs.col(nvec + v) = vecs.col(nvec + v) / onesize;
                    ++nonzero;
                }
            }
            nvec += one.data.nrow;
        }
    }
    //
    //   Check whether all of the vectors are exactly zero
    //
    if (nonzero == 0)
        return 0;

    //
    //   Reorthogonalize the vectors
    //
    JacobiSVD<MatrixXd> svd(vecs.block(0, 0, vecs.rows(), nvec), ComputeFullU);
    //Sort singular values and singular vectors
    VectorXd S = svd.singularValues();
    MatrixXd t_U = svd.matrixU();
    Linalg::sort<double>(S, t_U);

    //
    //   Throw away the linearly dependent guys
    //
    nproj = 0;
    for (k = 0; k < S.size(); ++k)
        if (S[k] / S[0] > 1e-2)
            ++nproj;

    U = t_U.block(0, 0, t_U.rows(), nproj);

    //
    //   Here is the celebrated result
    //
    proj -= U * U.transpose();

    return nproj;
}

//=============================================================================================================

std::vector<FiffProj> FiffProj::compute_from_raw(const FiffRawData& raw,
                                                 const MatrixXi& events,
                                                 int eventCode,
                                                 float tmin,
                                                 float tmax,
                                                 int nGrad,
                                                 int nMag,
                                                 int nEeg,
                                                 const std::map<std::string, double>& mapReject)
{
    float sfreq = raw.info.sfreq;
    int nchan = raw.info.nchan;

    int minSamp = static_cast<int>(std::round(tmin * sfreq));
    int maxSamp = static_cast<int>(std::round(tmax * sfreq));
    int ns = maxSamp - minSamp + 1;

    if (ns <= 0) {
        Log::warning("[FiffProj::compute_from_raw] Invalid time window.");
        return {};
    }

    // Collect matching epochs
    std::vector<MatrixXd> epochs;
    const auto reject = [&mapReject](const char* type) {
        const auto it = mapReject.find(type);
        return it == mapReject.end() ? 0.0 : it->second;
    };
    double gradReject = reject("grad");
    double magReject = reject("mag");
    double eegReject = reject("eeg");

    for (int k = 0; k < events.rows(); ++k) {
        if (events(k, 1) != 0 || events(k, 2) != eventCode)
            continue;

        int evSample = events(k, 0);
        int epochStart = evSample + minSamp;
        int epochEnd = evSample + maxSamp;

        if (epochStart < raw.first_samp || epochEnd > raw.last_samp)
            continue;

        MatrixXd epochData, epochTimes;
        if (!raw.read_raw_segment(epochData, epochTimes, epochStart, epochEnd))
            continue;

        // Simple peak-to-peak rejection
        bool ok = true;
        for (int c = 0; c < nchan && ok; ++c) {
            if (has_channel(raw.info.bads, raw.info.ch_names[c]))
                continue;
            double pp = epochData.row(c).maxCoeff() - epochData.row(c).minCoeff();
            if (raw.info.chs[c].kind == FIFFV_MEG_CH) {
                if (raw.info.chs[c].unit == FIFF_UNIT_T && magReject > 0 && pp > magReject)
                    ok = false;
                else if (raw.info.chs[c].unit != FIFF_UNIT_T && gradReject > 0 && pp > gradReject)
                    ok = false;
            } else if (raw.info.chs[c].kind == FIFFV_EEG_CH && eegReject > 0 && pp > eegReject) {
                ok = false;
            }
        }
        if (!ok)
            continue;

        epochs.push_back(epochData);
    }

    if (epochs.empty()) {
        Log::warning("[FiffProj::compute_from_raw] No valid epochs found for event {}", eventCode);
        return {};
    }

    Log::info("[FiffProj::compute_from_raw] {} epochs collected for event {}", epochs.size(), eventCode);

    // SVD of the uncentred data (MNE-C compute_cov_raw_epoch with remove_sample_mean = FALSE,
    // mne.compute_proj_epochs); removing the mean first yields a different subspace.
    MatrixXd data(nchan, epochs.size() * ns);
    for (int e = 0; e < std::ssize(epochs); ++e) {
        data.middleCols(e * ns, ns) = epochs[e];
    }
    const std::array<std::vector<int>, 3> groups = pcaChannelGroups(raw.info);
    const int nVec[3] = {nGrad, nMag, nEeg};
    const char* kinds[3] = {"grad", "mag", "eeg"};
    std::vector<FiffProj> projs;
    for (int g = 0; g < 3; ++g) {
        StringList descs;
        for (int v = 0; v < nVec[g]; ++v) {
            descs.push_back(std::format("PCA-{}-v{}", kinds[g], v + 1));
        }
        std::ranges::move(pcaProjectors(data, groups[g], raw.info.ch_names, nVec[g], descs), std::back_inserter(projs));
    }

    return projs;
}

//=============================================================================================================

std::vector<FiffProj> FiffProj::compute_from_evoked(const FiffEvoked& evoked, int nGrad, int nMag, int nEeg)
{
    if (evoked.data.cols() == 0 || evoked.times.size() == 0) {
        return {};
    }
    const std::string prefix = std::format("{:.3f}-{:.3f}", evoked.times(0), evoked.times(evoked.times.size() - 1));
    const std::array<std::vector<int>, 3> groups = pcaChannelGroups(evoked.info);
    const int nVec[3] = {nGrad, nMag, nEeg};
    const char* kinds[3] = {"planar", "axial", "eeg"};
    std::vector<FiffProj> projs;
    for (int g = 0; g < 3; ++g) {
        StringList descs;
        for (int v = 0; v < nVec[g]; ++v) {
            descs.push_back(std::format("{}-{}-PCA-{:02}", kinds[g], prefix, v + 1));
        }
        std::ranges::move(pcaProjectors(evoked.data, groups[g], evoked.info.ch_names, nVec[g], descs), std::back_inserter(projs));
    }
    return projs;
}
