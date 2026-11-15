//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_cov.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Florian Schlembach <fschlembach@web.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     October 2012
 * @brief    Implementation of @ref FiffCov: read / write of ``*-cov.fif`` files, projector application, regularization and whitening eigendecomposition.
 *
 * Round-trips with @c mne.Covariance in MNE-Python. Carries the matrix,
 * the channel names, the kind, the active SSP projectors and the
 * nfree degrees-of-freedom count needed by the source-localization
 * pipeline.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_cov.h"
#include "fiff_ch_name.h"
#include <memory>
#include <base/log.h>

#include <algorithm>
#include <iterator>

#include <vector>
#include <utility>

#include <base/log.h>
#include <base/string_utils.h>
#include "fiff_stream.h"
#include "fiff_raw_data.h"
#include "fiff_evoked_set.h"
#include "fiff_info_base.h"
#include "fiff_dir_node.h"
#include "fiff_file.h"

#include <math/linalg.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/SVD>

#include <stdexcept>
#include <cstdint>
#include <string_view>
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using mne::ErrorCode;
using mne::fail;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffCov::FiffCov()
: kind(-1)
, diag(false)
, dim(-1)
, nfree(-1)
{
    qRegisterMetaType<std::shared_ptr<mne::FiffCov>>();
    qRegisterMetaType<mne::FiffCov>();
}

//=============================================================================================================

FiffCov::FiffCov(const std::filesystem::path& path)
: FiffCov(read(path).value())
{
}

//=============================================================================================================

mne::Expected<FiffCov> FiffCov::read(const std::filesystem::path& path)
{
    auto stream = FiffStream::open(path);
    if (!stream) {
        return std::unexpected(stream.error());
    }
    FiffCov cov;
    if (!stream->read_cov(stream->dirtree(), FIFFV_MNE_NOISE_COV, cov)) {
        return fail(ErrorCode::NotFound, "No noise covariance in {}", mne::utf8_from_path(path));
    }
    return cov;
}

//=============================================================================================================

void FiffCov::clear()
{
    kind = -1;
    diag = false;
    dim = -1;
    names.clear();
    data = MatrixXd();
    projs.clear();
    bads.clear();
    nfree = -1;
    eig = VectorXd();
    eigvec = MatrixXd();
}

//=============================================================================================================

FiffCov FiffCov::pick_channels(const StringList& p_include, const StringList& p_exclude)
{
    RowVectorXi sel = FiffInfoBase::pick_channels(this->names, p_include, p_exclude);
    FiffCov res; //No deep copy here - since almost everything else is adapted anyway

    res.kind = this->kind;
    res.diag = this->diag;
    res.dim = sel.size();

    for (std::int32_t k = 0; k < sel.size(); ++k)
        res.names.push_back(this->names[sel(k)]);

    res.data.resize(res.dim, res.dim);
    for (std::int32_t i = 0; i < res.dim; ++i)
        for (std::int32_t j = 0; j < res.dim; ++j)
            res.data(i, j) = this->data(sel(i), sel(j));
    res.projs = this->projs;

    for (std::int32_t k = 0; k < static_cast<int>(this->bads.size()); ++k)
        if (mne::has_channel(res.names, this->bads[k]))
            res.bads.push_back(this->bads[k]);
    res.nfree = this->nfree;

    return res;
}

//=============================================================================================================

FiffCov FiffCov::prepare_noise_cov(const FiffInfo& p_Info, const StringList& p_ChNames) const
{
    FiffCov p_NoiseCov(*this);

    VectorXi C_ch_idx = VectorXi::Zero(p_NoiseCov.names.size());
    std::int32_t count = 0;
    for (std::int32_t i = 0; i < static_cast<int>(p_ChNames.size()); ++i) {
        std::int32_t idx = mne::channel_index(p_NoiseCov.names, p_ChNames[i]);
        if (idx > -1) {
            C_ch_idx[count] = idx;
            ++count;
        }
    }
    C_ch_idx.conservativeResize(count);

    MatrixXd C(count, count);

    if (!p_NoiseCov.diag)
        for (std::int32_t i = 0; i < count; ++i)
            for (std::int32_t j = 0; j < count; ++j)
                C(i, j) = p_NoiseCov.data(C_ch_idx(i), C_ch_idx(j));
    else {
        Log::warning("Warning in FiffCov::prepare_noise_cov: This has to be debugged - not done before!");
        C = MatrixXd::Zero(count, count);
        for (std::int32_t i = 0; i < count; ++i)
            C.diagonal()[i] = p_NoiseCov.data(C_ch_idx(i), 0);
    }

    MatrixXd proj;
    std::int32_t ncomp = p_Info.make_projector(proj, p_ChNames);

    //Create the projection operator
    if (ncomp > 0 && proj.rows() == count) {
        Log::info("Created an SSP operator (subspace dimension = {})\n", ncomp);
        C = proj * (C * proj.transpose());
    } else {
        Log::warning("Warning in FiffCov::prepare_noise_cov: No projections applied since no projectors specified or projector dimensions do not match!");
    }

    RowVectorXi pick_meg = p_Info.pick_types(true, false, false, {}, p_Info.bads);
    RowVectorXi pick_eeg = p_Info.pick_types(false, true, false, {}, p_Info.bads);

    StringList meg_names, eeg_names;

    for (std::int32_t i = 0; i < pick_meg.size(); ++i)
        meg_names.push_back(p_Info.chs[pick_meg[i]].ch_name);
    VectorXi C_meg_idx = VectorXi::Zero(p_NoiseCov.names.size());
    count = 0;
    for (std::int32_t k = 0; k < C.rows(); ++k) {
        if (mne::channel_index(meg_names, p_ChNames[k]) > -1) {
            C_meg_idx[count] = k;
            ++count;
        }
    }
    if (count > 0)
        C_meg_idx.conservativeResize(count);
    else
        C_meg_idx = VectorXi();

    //
    for (std::int32_t i = 0; i < pick_eeg.size(); ++i)
        eeg_names.push_back(p_Info.chs[pick_eeg(0, i)].ch_name);
    VectorXi C_eeg_idx = VectorXi::Zero(p_NoiseCov.names.size());
    count = 0;
    for (std::int32_t k = 0; k < C.rows(); ++k) {
        if (mne::channel_index(eeg_names, p_ChNames[k]) > -1) {
            C_eeg_idx[count] = k;
            ++count;
        }
    }

    if (count > 0)
        C_eeg_idx.conservativeResize(count);
    else
        C_eeg_idx = VectorXi();

    bool has_meg = C_meg_idx.size() > 0;
    bool has_eeg = C_eeg_idx.size() > 0;

    MatrixXd C_meg, C_eeg;
    VectorXd C_meg_eig, C_eeg_eig;
    MatrixXd C_meg_eigvec, C_eeg_eigvec;
    if (has_meg) {
        count = C_meg_idx.rows();
        C_meg = MatrixXd(count, count);
        for (std::int32_t i = 0; i < count; ++i)
            for (std::int32_t j = 0; j < count; ++j)
                C_meg(i, j) = C(C_meg_idx(i), C_meg_idx(j));
        Linalg::get_whitener(C_meg, false, "MEG", C_meg_eig, C_meg_eigvec);
    }

    if (has_eeg) {
        count = C_eeg_idx.rows();
        C_eeg = MatrixXd(count, count);
        for (std::int32_t i = 0; i < count; ++i)
            for (std::int32_t j = 0; j < count; ++j)
                C_eeg(i, j) = C(C_eeg_idx(i), C_eeg_idx(j));
        Linalg::get_whitener(C_eeg, false, "EEG", C_eeg_eig, C_eeg_eigvec);
    }

    std::int32_t n_chan = static_cast<int>(p_ChNames.size());
    p_NoiseCov.eigvec = MatrixXd::Zero(n_chan, n_chan);
    p_NoiseCov.eig = VectorXd::Zero(n_chan);

    if (has_meg) {
        for (std::int32_t i = 0; i < C_meg_idx.rows(); ++i)
            for (std::int32_t j = 0; j < C_meg_idx.rows(); ++j)
                p_NoiseCov.eigvec(C_meg_idx[i], C_meg_idx[j]) = C_meg_eigvec(i, j);
        for (std::int32_t i = 0; i < C_meg_idx.rows(); ++i)
            p_NoiseCov.eig(C_meg_idx[i]) = C_meg_eig[i];
    }
    if (has_eeg) {
        for (std::int32_t i = 0; i < C_eeg_idx.rows(); ++i)
            for (std::int32_t j = 0; j < C_eeg_idx.rows(); ++j)
                p_NoiseCov.eigvec(C_eeg_idx[i], C_eeg_idx[j]) = C_eeg_eigvec(i, j);
        for (std::int32_t i = 0; i < C_eeg_idx.rows(); ++i)
            p_NoiseCov.eig(C_eeg_idx[i]) = C_eeg_eig[i];
    }

    if (C_meg_idx.size() + C_eeg_idx.size() != n_chan) {
        Log::warning("FiffCov::prepare_noise_cov: {} MEG + {} EEG channels != {} total (unclassified channels present)", (long long)C_meg_idx.size(), (long long)C_eeg_idx.size(), n_chan);
    }

    p_NoiseCov.data = C;
    p_NoiseCov.dim = static_cast<int>(p_ChNames.size());
    p_NoiseCov.diag = false;
    p_NoiseCov.names = p_ChNames;

    return p_NoiseCov;
}

//=============================================================================================================

FiffCov FiffCov::regularize(const FiffInfo& p_info, double p_fRegMag, double p_fRegGrad, double p_fRegEeg, bool p_bProj, StringList p_exclude) const
{
    FiffCov cov(*this);

    if (p_exclude.size() == 0) {
        p_exclude = p_info.bads;
        for (std::int32_t i = 0; i < static_cast<int>(cov.bads.size()); ++i)
            if (!mne::has_channel(p_exclude, cov.bads[i]))
                p_exclude.push_back(cov.bads[i]);
    }

    //Allways exclude all STI channels from covariance computation
    for (int i = 0; i < std::ssize(p_info.chs); i++) {
        if (p_info.chs[i].kind == FIFFV_STIM_CH) {
            p_exclude.push_back(p_info.chs[i].ch_name);
        }
    }

    RowVectorXi sel_eeg = p_info.pick_types(false, true, false, {}, p_exclude);
    RowVectorXi sel_mag = p_info.pick_types(std::string("mag"), false, false, {}, p_exclude);
    RowVectorXi sel_grad = p_info.pick_types(std::string("grad"), false, false, {}, p_exclude);

    StringList info_ch_names = p_info.ch_names;
    StringList ch_names_eeg, ch_names_mag, ch_names_grad;
    for (std::int32_t i = 0; i < sel_eeg.size(); ++i)
        ch_names_eeg.push_back(info_ch_names[sel_eeg(i)]);
    for (std::int32_t i = 0; i < sel_mag.size(); ++i)
        ch_names_mag.push_back(info_ch_names[sel_mag(i)]);
    for (std::int32_t i = 0; i < sel_grad.size(); ++i)
        ch_names_grad.push_back(info_ch_names[sel_grad(i)]);

    // This actually removes bad channels from the cov, which is not backward
    // compatible, so let's leave all channels in
    FiffCov cov_good = cov.pick_channels(info_ch_names, p_exclude);
    StringList ch_names = cov_good.names;

    std::vector<std::int32_t> idx_eeg, idx_mag, idx_grad;
    for (std::int32_t i = 0; i < static_cast<int>(ch_names.size()); ++i) {
        if (mne::has_channel(ch_names_eeg, ch_names[i]))
            idx_eeg.push_back(i);
        else if (mne::has_channel(ch_names_mag, ch_names[i]))
            idx_mag.push_back(i);
        else if (mne::has_channel(ch_names_grad, ch_names[i]))
            idx_grad.push_back(i);
    }

    MatrixXd C(cov_good.data);

    //Check dimension consistency (channels not classified as EEG/MAG/GRAD, e.g. EOG/MISC, are expected)
    if (static_cast<unsigned>(C.rows()) != idx_eeg.size() + idx_mag.size() + idx_grad.size()) {
        Log::warning("FiffCov::regularize: {} channels in cov but only {} classified as EEG/MAG/GRAD (others will not be regularized)", C.rows(), idx_eeg.size() + idx_mag.size() + idx_grad.size());
    }

    std::vector<FiffProj> t_listProjs;
    if (p_bProj) {
        t_listProjs.reserve(p_info.projs.size() + cov_good.projs.size());
        t_listProjs.insert(t_listProjs.end(), p_info.projs.begin(), p_info.projs.end());
        t_listProjs.insert(t_listProjs.end(), cov_good.projs.begin(), cov_good.projs.end());
        FiffProj::activate_projs(t_listProjs);
    }

    struct Regularization
    {
        std::string_view desc;
        double reg;
        const std::vector<std::int32_t>& idx;
    };
    const Regularization regData[] = {{"EEG", p_fRegEeg, idx_eeg}, {"GRAD", p_fRegGrad, idx_grad}, {"MAG", p_fRegMag, idx_mag}};

    for (const auto& [desc, reg, idx] : regData) {
        if (idx.size() == 0 || reg == 0.0)
            Log::info("\tNothing to regularize within {} data.\n", desc);
        else {
            Log::info("\tRegularize {}: {:f}\n", desc, reg);
            MatrixXd this_C(idx.size(), idx.size());
            for (std::uint32_t i = 0; i < idx.size(); ++i)
                for (std::uint32_t j = 0; j < idx.size(); ++j)
                    this_C(i, j) = cov_good.data(idx[i], idx[j]);

            MatrixXd U;
            // Only assigned when projecting, but read again below to decide
            // whether the SSP operator has to be undone. Without projection
            // there are no components to remove.
            std::int32_t ncomp = 0;
            if (p_bProj) {
                StringList this_ch_names;
                for (std::uint32_t k = 0; k < idx.size(); ++k)
                    this_ch_names.push_back(ch_names[idx[k]]);

                MatrixXd P;
                ncomp = FiffProj::make_projector(t_listProjs, this_ch_names, P); //ToDo: Synchronize with mne-python and debug

                JacobiSVD<MatrixXd> svd(P, ComputeFullU);
                //Sort singular values and singular vectors
                VectorXd t_s = svd.singularValues();
                MatrixXd t_U = svd.matrixU();
                Linalg::sort<double>(t_s, t_U);

                U = t_U.block(0, 0, t_U.rows(), t_U.cols() - ncomp);

                if (ncomp > 0) {
                    Log::info("\tCreated an SSP operator for {} (dimension = {}).\n", desc, ncomp);
                    this_C = U.transpose() * (this_C * U);
                }
            }

            double sigma = this_C.diagonal().mean();
            this_C.diagonal() = this_C.diagonal().array() + reg * sigma; // modify diag inplace
            if (p_bProj && ncomp > 0)
                this_C = U * (this_C * U.transpose());

            for (std::int32_t i = 0; i < this_C.rows(); ++i)
                for (std::int32_t j = 0; j < this_C.cols(); ++j)
                    C(idx[i], idx[j]) = this_C(i, j);
        }
    }

    // Put data back in correct locations
    RowVectorXi idx = FiffInfo::pick_channels(cov.names, info_ch_names, p_exclude);
    for (std::int32_t i = 0; i < idx.size(); ++i)
        for (std::int32_t j = 0; j < idx.size(); ++j)
            cov.data(idx[i], idx[j]) = C(i, j);

    return cov;
}

//=============================================================================================================

FiffCov FiffCov::compute_from_epochs(const FiffRawData& raw,
                                     const MatrixXi& events,
                                     const std::vector<int>& eventCodes,
                                     float tmin,
                                     float tmax,
                                     float bmin,
                                     float bmax,
                                     bool doBaseline,
                                     bool removeMean,
                                     unsigned int ignoreMask,
                                     float delay,
                                     const RejectionParams* rej)
{
    FiffCov cov;
    float sfreq = raw.info.sfreq;
    int nchan = raw.info.nchan;

    int minSamp = static_cast<int>(std::round(tmin * sfreq));
    int maxSamp = static_cast<int>(std::round(tmax * sfreq));
    int ns = maxSamp - minSamp + 1;
    int delaySamp = static_cast<int>(std::round(delay * sfreq));

    if (ns <= 0) {
        Log::warning("[FiffCov::compute_from_epochs] Invalid time window.");
        return cov;
    }

    int bminSamp = 0, bmaxSamp = 0;
    if (doBaseline) {
        bminSamp = static_cast<int>(std::round(bmin * sfreq)) - minSamp;
        bmaxSamp = static_cast<int>(std::round(bmax * sfreq)) - minSamp;
    }

    // The accepted epochs, grouped by event code
    std::vector<std::vector<MatrixXd>> epochs(eventCodes.size());
    for (int k = 0; k < events.rows(); ++k) {
        int evFrom = events(k, 1) & ~static_cast<int>(ignoreMask);
        int evTo = events(k, 2) & ~static_cast<int>(ignoreMask);

        const auto codeIt = std::ranges::find(eventCodes, evTo);
        const int iCode = evFrom == 0 && codeIt != eventCodes.end() ? static_cast<int>(codeIt - eventCodes.begin()) : -1;
        if (iCode < 0)
            continue;

        int evSample = events(k, 0);
        int epochStart = evSample + delaySamp + minSamp;
        int epochEnd = evSample + delaySamp + maxSamp;

        if (epochStart < raw.first_samp || epochEnd > raw.last_samp)
            continue;

        MatrixXd epochData;
        MatrixXd epochTimes;
        if (!raw.read_raw_segment(epochData, epochTimes, epochStart, epochEnd))
            continue;
        std::string reason;
        if (rej && !FiffEvokedSet::checkArtifacts(epochData, raw.info, raw.info.bads, *rej, reason)) {
            Log::info("[FiffCov::compute_from_epochs] Rejected epoch at {} {}", evSample, reason);
            continue;
        }

        // Baseline subtraction
        if (doBaseline) {
            int bminIdx = qMax(0, bminSamp);
            int bmaxIdx = qMin(static_cast<int>(epochData.cols()) - 1, bmaxSamp);
            if (bmaxIdx >= bminIdx) {
                int nBase = bmaxIdx - bminIdx + 1;
                for (int c = 0; c < nchan; ++c) {
                    double baseVal = epochData.row(c).segment(bminIdx, nBase).mean();
                    epochData.row(c).array() -= baseVal;
                }
            }
        }

        epochs[iCode].push_back(epochData);
    }

    return compute_from_epochs(epochs, raw.info, removeMean);
}

//=============================================================================================================

FiffCov FiffCov::compute_from_epochs(const std::vector<std::vector<MatrixXd>>& epochs,
                                     const FiffInfo& info,
                                     bool removeMean)
{
    FiffCov cov;
    const int nchan = info.nchan;
    MatrixXd covAccum = MatrixXd::Zero(nchan, nchan);
    int totalSamples = 0;
    int nEpochs = 0;
    double norm = 0.0;
    for (const std::vector<MatrixXd>& codeEpochs : epochs) {
        if (codeEpochs.empty())
            continue;
        MatrixXd sum = MatrixXd::Zero(nchan, codeEpochs.front().cols());
        for (const MatrixXd& epoch : codeEpochs) {
            covAccum += epoch * epoch.transpose();
            sum += epoch;
            totalSamples += static_cast<int>(epoch.cols());
        }
        nEpochs += static_cast<int>(codeEpochs.size());
        if (removeMean) {
            // As mne.compute_covariance(keep_sample_mean=False) and MNE-C compute_cov: subtract each code's evoked response
            covAccum -= sum * sum.transpose() / static_cast<double>(codeEpochs.size());
            norm += static_cast<double>(sum.cols()) * (codeEpochs.size() - 1);
        }
    }

    if (totalSamples < 2) {
        Log::warning("[FiffCov::compute_from_epochs] Not enough data.");
        return cov;
    }
    if (removeMean && norm <= 0.0) {
        Log::warning("[FiffCov::compute_from_epochs] Removing the sample mean needs two epochs of one event code.");
        return cov;
    }
    cov.data = covAccum / (removeMean ? norm : static_cast<double>(totalSamples - 1));

    cov.kind = FIFFV_MNE_NOISE_COV;
    cov.dim = nchan;
    cov.names = info.ch_names;
    cov.nfree = totalSamples - 1;
    cov.bads = info.bads;
    cov.projs = info.projs;

    Log::info("[FiffCov::compute_from_epochs] Computed: {} channels, {} epochs, {} total samples.", nchan, nEpochs, totalSamples);

    return cov;
}

//=============================================================================================================

mne::Expected<void> FiffCov::save(const std::filesystem::path& path) const
{
    auto pStream = path.empty() ? nullptr : FiffStream::start_file(path);
    if (!pStream) {
        return fail(ErrorCode::Io, "Cannot write the covariance to '{}'", mne::utf8_from_path(path));
    }

    pStream->start_block(FIFFB_MEAS);
    pStream->write_id(FIFF_BLOCK_ID);
    pStream->write_cov(*this);
    pStream->end_block(FIFFB_MEAS);
    pStream->end_file();
    pStream->close();

    return {};
}

//=============================================================================================================

FiffCov FiffCov::compute_grand_average(const std::vector<FiffCov>& covs)
{
    FiffCov grandCov;

    if (covs.empty()) {
        Log::warning("[FiffCov::compute_grand_average] No covariance matrices provided.");
        return grandCov;
    }

    grandCov = covs[0];
    MatrixXd sumCov = grandCov.data * static_cast<double>(grandCov.nfree);
    int totalNfree = grandCov.nfree;

    for (int k = 1; k < std::ssize(covs); ++k) {
        if (covs[k].dim != grandCov.dim) {
            Log::warning("[FiffCov::compute_grand_average] Dimension mismatch.");
            return FiffCov();
        }
        sumCov += covs[k].data * static_cast<double>(covs[k].nfree);
        totalNfree += covs[k].nfree;
    }

    grandCov.data = sumCov / static_cast<double>(totalNfree);
    grandCov.nfree = totalNfree;

    return grandCov;
}
