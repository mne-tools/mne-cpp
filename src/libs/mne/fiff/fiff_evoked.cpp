//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_evoked.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Florian Schlembach <fschlembach@web.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     July 2012
 * @brief    Implementation of @ref FiffEvoked: read / write of one FIFFB_EVOKED block plus baseline / projector / pick operations.
 *
 * Drop-in counterpart of @c mne.Evoked in MNE-Python.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_evoked.h"

#include "fiff_ch_name.h"
#include "fiff_stream.h"
#include "fiff_tag.h"
#include "fiff_dir_node.h"
#include "fiff_evoked_file.h"
#include <mne/base/log.h>
#include <mne/base/string_utils.h>
#include <mne/math/numerics.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <vector>
#include <utility>
#include <cmath>
#include <format>
#include <iterator>
#include <string_view>
#include <cstdint>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffEvoked::FiffEvoked()
: nave(-1)
, aspect_kind(-1)
, first(-1)
, last(-1)
, baseline(std::make_pair(-1.0f, -1.0f))
{
}

//=============================================================================================================

FiffEvoked::FiffEvoked(const std::filesystem::path& path,
                       const Condition& condition,
                       const Baseline& baseline,
                       bool proj,
                       fiff_int_t kind)
: FiffEvoked(read(path, condition, baseline, proj, kind).value())
{
}

//=============================================================================================================

FiffEvoked::FiffEvoked(const FiffEvoked& p_FiffEvoked)
: info(p_FiffEvoked.info)
, nave(p_FiffEvoked.nave)
, aspect_kind(p_FiffEvoked.aspect_kind)
, first(p_FiffEvoked.first)
, last(p_FiffEvoked.last)
, comment(p_FiffEvoked.comment)
, times(p_FiffEvoked.times)
, data(p_FiffEvoked.data)
, proj(p_FiffEvoked.proj)
, baseline(p_FiffEvoked.baseline)
{
}

//=============================================================================================================

FiffEvoked::~FiffEvoked()
{
}

//=============================================================================================================

void FiffEvoked::clear()
{
    info.clear();
    nave = -1;
    aspect_kind = -1;
    first = -1;
    last = -1;
    comment.clear();
    times = RowVectorXf();
    data = MatrixXd();
    proj = MatrixXd();
}

//=============================================================================================================

FiffEvoked FiffEvoked::pick_channels(const StringList& include,
                                     const StringList& exclude) const
{
    if (include.size() == 0 && exclude.size() == 0)
        return FiffEvoked(*this);

    RowVectorXi sel = FiffInfo::pick_channels(this->info.ch_names, include, exclude);
    if (sel.cols() == 0) {
        Log::warning("Warning : No channels match the selection.\n");
        return FiffEvoked(*this);
    }

    FiffEvoked res(*this);
    //
    //   Modify the measurement info
    //
    res.info = FiffInfo(res.info.pick_info(sel));
    //
    //   Create the reduced data set
    //
    MatrixXd selBlock = MatrixXd::Zero(sel.cols(), res.data.cols());
    for (std::int32_t l = 0; l < sel.cols(); ++l) {
        if (sel(0, l) < res.data.rows()) {
            selBlock.block(l, 0, 1, selBlock.cols()) = res.data.block(sel(0, l), 0, 1, selBlock.cols());
        } else {
            Log::warning("FiffEvoked::pick_channels - Warning : Selected channel index out of bound.\n");
        }
    }
    res.data.resize(sel.cols(), res.data.cols());
    res.data = selBlock;

    return res;
}

//=============================================================================================================

Expected<FiffEvoked> FiffEvoked::read(const std::filesystem::path& path,
                                      const Condition& condition,
                                      const Baseline& baseline,
                                      bool proj,
                                      fiff_int_t kind)
{
    auto file = FiffEvokedFile::open(path);
    if (!file) {
        return std::unexpected(file.error());
    }
    const auto set = file->find(condition, kind);
    if (!set) {
        return std::unexpected(set.error());
    }
    return file->read(*set, baseline, proj);
}

//=============================================================================================================

Expected<FiffEvokedFile> FiffEvokedFile::open(const std::filesystem::path& path)
{
    auto opened = FiffStream::open(path);
    if (!opened) {
        return std::unexpected(opened.error());
    }
    FiffEvokedFile file(std::move(*opened), utf8_from_path(path));
    FiffStream& stream = file.m_stream;
    Log::info("Reading {} ...\n", file.m_file);

    std::shared_ptr<FiffDirNode> meas;
    if (stream.dirtree()->dir_tree_find(FIFFB_MEAS).empty()) {
        return fail(ErrorCode::NotFound, "No measurement in {}", file.m_file);
    }
    if (!stream.read_meas_info(stream.dirtree(), file.info, meas)) {
        return fail(ErrorCode::Format, "No measurement info in {}", file.m_file);
    }
    file.info.filename = stream.stream_name();
    if (meas->dir_tree_find(FIFFB_PROCESSED_DATA).empty()) {
        return fail(ErrorCode::NotFound, "No processed data in {}", file.m_file);
    }
    file.m_sets = meas->dir_tree_find(FIFFB_EVOKED);
    if (file.m_sets.empty()) {
        return fail(ErrorCode::NotFound, "No evoked data in {}", file.m_file);
    }
    std::string listing;
    stream.get_evoked_entries(file.m_sets, file.m_comments, file.m_kinds, listing);
    return file;
}

//=============================================================================================================

Expected<int> FiffEvokedFile::find(const FiffEvoked::Condition& condition, fiff_int_t kind) const
{
    if (const int* index = std::get_if<int>(&condition)) {
        if (*index >= 0 && *index < std::ssize(m_sets)) {
            return *index;
        }
    } else {
        const std::string& wanted = std::get<std::string>(condition);
        for (std::size_t i = 0; i < m_comments.size(); ++i) {
            if (m_comments[i] == wanted && m_kinds[i] == kind) {
                return static_cast<int>(i);
            }
        }
    }
    return fail(ErrorCode::NotFound, "No evoked data set {} of kind {} in {}",
                std::visit([](const auto& c) { return std::format("{}", c); }, condition), kind, m_file);
}

//=============================================================================================================

Expected<FiffEvoked> FiffEvokedFile::read(int set, const FiffEvoked::Baseline& baseline, bool proj)
{
    FiffInfo evokedInfo = this->info;
    const std::string& file = m_file;
    FiffEvoked p_FiffEvoked;
    std::shared_ptr<FiffDirNode> my_evoked = m_sets[set];

    //
    //   Identify the aspects
    //
    std::vector<std::shared_ptr<FiffDirNode>> aspects = my_evoked->dir_tree_find(FIFFB_ASPECT);
    if (aspects.empty()) {
        return fail(ErrorCode::Format, "Evoked data set {} without data in {}", set, file);
    }

    if (aspects.size() > 1)
        Log::info("\tMultiple ({}) aspects found. Taking first one.\n", aspects.size());

    const std::shared_ptr<FiffDirNode>& my_aspect = aspects[0];

    //
    //   Now find the data in the evoked block
    //
    fiff_int_t nchan = 0;
    float sfreq = -1.0f;
    std::vector<FiffChInfo> chs;
    fiff_int_t tag_kind, pos, first = 0, last = 0;
    std::optional<FiffTag> t_pTag;
    std::string comment;
    std::int32_t k;
    for (k = 0; k < my_evoked->nent(); ++k) {
        tag_kind = my_evoked->dir[k].kind;
        pos = my_evoked->dir[k].pos;
        switch (tag_kind) {
            case FIFF_COMMENT:
                t_pTag = m_stream.read_tag(pos);
                comment = t_pTag->to_string();
                break;
            case FIFF_FIRST_SAMPLE:
                t_pTag = m_stream.read_tag(pos);
                first = *t_pTag->to_int();
                break;
            case FIFF_LAST_SAMPLE:
                t_pTag = m_stream.read_tag(pos);
                last = *t_pTag->to_int();
                break;
            case FIFF_NCHAN:
                t_pTag = m_stream.read_tag(pos);
                nchan = *t_pTag->to_int();
                break;
            case FIFF_SFREQ:
                t_pTag = m_stream.read_tag(pos);
                sfreq = *t_pTag->to_float();
                break;
            case FIFF_CH_INFO:
                t_pTag = m_stream.read_tag(pos);
                chs.push_back(t_pTag->to_ch_info());
                break;
        }
    }
    if (comment.empty())
        comment = "No comment";

    //
    //   Local channel information?
    //
    if (nchan > 0) {
        if (chs.size() == 0) {
            return fail(ErrorCode::Format, "Local channel information missing in {}", file);
        }
        if (std::ssize(chs) != nchan) {
            return fail(ErrorCode::Format, "{} channels but {} channel definitions in {}", nchan, chs.size(), file);
        }
        evokedInfo.chs = chs;
        evokedInfo.nchan = nchan;
        Log::info("\tFound channel information in evoked data. nchan = {}\n", nchan);
        if (sfreq > 0.0f)
            evokedInfo.sfreq = sfreq;
    }
    std::int32_t nsamp = last - first + 1;
    Log::info("\tFound the data of interest:\n");
    Log::info("\t\tt = {:10.2f} ... {:10.2f} ms ({})\n", 1000 * static_cast<float>(first) / evokedInfo.sfreq, 1000 * static_cast<float>(last) / evokedInfo.sfreq, comment);
    if (evokedInfo.comps.size() > 0)
        Log::info("\t\t{} CTF compensation matrices available\n", evokedInfo.comps.size());

    //
    // Read the data in the aspect block
    //
    fiff_int_t aspect_kind = -1;
    fiff_int_t nave = -1;
    std::vector<FiffTag> epoch;
    for (k = 0; k < my_aspect->nent(); ++k) {
        tag_kind = my_aspect->dir[k].kind;
        pos = my_aspect->dir[k].pos;

        switch (tag_kind) {
            case FIFF_COMMENT:
                t_pTag = m_stream.read_tag(pos);
                comment = t_pTag->to_string();
                break;
            case FIFF_ASPECT_KIND:
                t_pTag = m_stream.read_tag(pos);
                aspect_kind = *t_pTag->to_int();
                break;
            case FIFF_NAVE:
                t_pTag = m_stream.read_tag(pos);
                nave = *t_pTag->to_int();
                break;
            case FIFF_EPOCH:
                t_pTag = m_stream.read_tag(pos);
                epoch.push_back(std::move(*t_pTag));
                break;
        }
    }
    if (nave == -1)
        nave = 1;
    Log::info("\t\tnave = {} - aspect type = {}\n", nave, aspect_kind);

    std::int32_t nepoch = static_cast<int>(epoch.size());
    if (nepoch != 1 && nepoch != evokedInfo.nchan) {
        return fail(ErrorCode::Format, "{} epoch tags for {} channels in {}", nepoch, evokedInfo.nchan, file);
    }
    MatrixXd all_data;
    if (nepoch == 1) {
        //
        //   Only one epoch
        //
        all_data = epoch[0].to_float_matrix().cast<double>();
        all_data.transposeInPlace();
        //
        //   May need a transpose if the number of channels is one
        //
        if (all_data.cols() == 1 && evokedInfo.nchan == 1)
            all_data.transposeInPlace();
    } else {
        //
        //   Put the old style epochs together: one channel per tag
        //
        all_data.resize(nepoch, static_cast<int>(epoch[0].size() / sizeof(float)));
        for (k = 0; k < nepoch; ++k) {
            const int n = static_cast<int>(epoch[k].size() / sizeof(float));
            if (n != all_data.cols()) {
                return fail(ErrorCode::Format, "Old style epochs of lengths {} and {} in {}", n, all_data.cols(), file);
            }
            all_data.row(k) = Map<const RowVectorXf>(epoch[k].to_float(), n).cast<double>();
        }
    }
    if (all_data.cols() != nsamp) {
        return fail(ErrorCode::Format, "{} samples instead of {} in {}", all_data.cols(), nsamp, file);
    }

    //
    //   Calibrate
    //
    Log::info("\n\tPreprocessing...\n");
    Log::info("\t{} channels remain after picking\n", evokedInfo.nchan);

    using T = Eigen::Triplet<double>;
    std::vector<T> tripletList;
    tripletList.reserve(evokedInfo.nchan);
    for (k = 0; k < evokedInfo.nchan; ++k)
        tripletList.push_back(T(k, k, evokedInfo.chs[k].cal));
    SparseMatrix<double> cals(evokedInfo.nchan, evokedInfo.nchan);
    cals.setFromTriplets(tripletList.begin(), tripletList.end());

    all_data = cals * all_data;

    RowVectorXf times = RowVectorXf(last - first + 1);
    for (k = 0; k < times.size(); ++k)
        times[k] = static_cast<float>(first + k) / evokedInfo.sfreq;

    //
    // Set up projection
    //
    if (evokedInfo.projs.size() == 0 || !proj) {
        Log::info("\tNo projector specified for these data.\n");
        p_FiffEvoked.proj = MatrixXd();
    } else {
        //   Create the projector
        MatrixXd projection;
        std::int32_t nproj = evokedInfo.make_projector(projection);
        if (nproj == 0) {
            Log::warning("\tThe projection vectors do not apply to these channels\n");
            p_FiffEvoked.proj = MatrixXd();
        } else {
            Log::info("\tCreated an SSP operator (subspace dimension = {})\n", nproj);
            p_FiffEvoked.proj = projection;
        }

        //   The projection items have been activated
        FiffProj::activate_projs(evokedInfo.projs);
    }

    if (p_FiffEvoked.proj.rows() > 0) {
        all_data = p_FiffEvoked.proj * all_data;
        Log::info("\tSSP projectors applied to the evoked data\n");
    }

    // Put it all together
    p_FiffEvoked.info = evokedInfo;
    p_FiffEvoked.nave = nave;
    p_FiffEvoked.aspect_kind = aspect_kind;
    p_FiffEvoked.first = first;
    p_FiffEvoked.last = last;
    p_FiffEvoked.comment = std::move(comment);
    p_FiffEvoked.times = times;
    p_FiffEvoked.data = all_data;

    if (baseline && baseline->first != baseline->second) {
        std::pair<float, float> window(baseline->first, baseline->second);
        p_FiffEvoked.applyBaselineCorrection(window);
    } else {
        Log::info("\tNo baseline correction applied\n");
    }

    return p_FiffEvoked;
}

//=============================================================================================================

void FiffEvoked::setInfo(const FiffInfo& p_info,
                         bool applyProj)
{
    info = p_info;
    //
    // Set up projection
    //
    if (info.projs.size() == 0 || !applyProj) {
        Log::info("\tNo projector specified for these data.\n");
        this->proj = MatrixXd();
    } else {
        //   Create the projector
        MatrixXd projection;
        std::int32_t nproj = info.make_projector(projection);
        if (nproj == 0) {
            Log::warning("\tThe projection vectors do not apply to these channels\n");
            this->proj = MatrixXd();
        } else {
            Log::info("\tCreated an SSP operator (subspace dimension = {})\n", nproj);
            this->proj = projection;
        }

        //   The projection items have been activated
        FiffProj::activate_projs(info.projs);
    }
}

//=============================================================================================================

FiffEvoked& FiffEvoked::operator+=(const MatrixXd& newData)
{
    //Init matrix if necessary
    if (nave == -1 || nave == 0) {
        data = MatrixXd::Zero(newData.rows(), newData.cols());
    }

    if (data.cols() == newData.cols() && data.rows() == newData.rows()) {
        //Revert old averaging
        data = data * nave;

        //Do new averaging
        data += newData;
        if (nave <= 0) {
            nave = 1;
        } else {
            nave++;
        }

        data /= nave;
    }

    return *this;
}

//=============================================================================================================

FiffEvoked FiffEvoked::combine(const std::vector<FiffEvoked>& evokeds, CombineWeights weights)
{
    VectorXd w(evokeds.size());
    double naveSum = 0.0;
    for (const FiffEvoked& e : evokeds) {
        naveSum += e.nave;
    }
    for (int i = 0; i < std::ssize(evokeds); ++i) {
        w(i) = weights == CombineWeights::Nave ? evokeds[i].nave / naveSum
                                               : 1.0 / static_cast<double>(evokeds.size());
    }
    return combine(evokeds, w);
}

//=============================================================================================================

FiffEvoked FiffEvoked::combine(const std::vector<FiffEvoked>& evokeds, const VectorXd& weights)
{
    if (evokeds.empty() || weights.size() != std::ssize(evokeds)) {
        Log::warning("[FiffEvoked::combine] weights must be the same size as the evoked list.");
        return FiffEvoked();
    }
    const FiffEvoked& ref = evokeds.front();
    for (const FiffEvoked& e : evokeds) {
        if (e.info.ch_names != ref.info.ch_names || e.times.size() != ref.times.size() || (e.times - ref.times).cwiseAbs().maxCoeff() > 1e-6f) {
            Log::warning("[FiffEvoked::combine] All evoked data must have the same channels and times.");
            return FiffEvoked();
        }
    }

    FiffEvoked res(ref);
    res.data = MatrixXd::Zero(ref.data.rows(), ref.data.cols());
    double invNave = 0.0;
    std::string comment;
    for (int i = 0; i < std::ssize(evokeds); ++i) {
        const FiffEvoked& e = evokeds[i];
        const double w = weights(i);
        res.data += w * e.data;
        invNave += w * w / e.nave;
        for (const std::string& bad : e.info.bads) {
            if (!mne::has_channel(res.info.bads, bad)) {
                res.info.bads.push_back(bad);
            }
        }

        const bool unitWeight = std::abs(std::abs(w) - 1.0) <= 1e-8 + 1e-5;
        const std::string weight = unitWeight ? std::string() : std::format("{:.3f} \u00d7 ", std::abs(w));
        const std::string_view sign = w >= 0.0 ? (i == 0 ? "" : " + ") : (i == 0 ? "-" : " - ");
        if (e.comment.contains(" + ")) {
            std::format_to(std::back_inserter(comment), "{}{}({})", sign, weight, e.comment);
        } else {
            std::format_to(std::back_inserter(comment), "{}{}{}", sign, weight, e.comment.empty() ? std::string_view("unknown") : std::string_view(e.comment));
        }
    }
    for (std::size_t at = comment.find(" - - "); at != std::string::npos; at = comment.find(" - - ", at + 3)) {
        comment.replace(at, 5, " + ");
    }
    res.comment = std::move(comment);
    res.nave = static_cast<fiff_int_t>(std::lround(1.0 / invNave));
    return res;
}

//=============================================================================================================

void FiffEvoked::applyBaselineCorrection(std::pair<float, float>& p_baseline)
{
    // Skip baseline correction if sentinel value (-1, -1) or equal bounds are passed
    if (p_baseline.first == p_baseline.second) {
        Log::info("\tNo baseline correction applied\n");
        return;
    }

    // Run baseline correction
    Log::info("Applying baseline correction ... (mode: mean)\n");
    this->data = Numerics::rescale(this->data, this->times, p_baseline, "mean");
    this->baseline = p_baseline;
}
