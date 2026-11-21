//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     mne_proj_op.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     January 2017
 * @brief    Implementation of @ref mne::MNEProjOp.
 *
 * Implements list management, the orthonormalisation that turns the
 * user-supplied SSP vectors into a projector and the in-place application
 * to dense matrices or @ref mne::MNENamedMatrix instances.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_proj_op.h"

#include "mne_proj_item.h"
#include "mne_cov_matrix.h"
#include "mne_named_vector.h"
#include "mne_named_matrix.h"
#include <fiff/fiff_constants.h>
#include <fiff/fiff_ch_name.h>
#include <base/log.h>
#include <base/string_utils.h>
#include <fiff/fiff_tag.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SVD>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <algorithm>
#include <sstream>
#include <string_view>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEProjOp::MNEProjOp()
: nvec(0)
{
}

//=============================================================================================================

MNEProjOp::~MNEProjOp()
{
}

//=============================================================================================================

void MNEProjOp::free_proj()
{
    proj_data.resize(0, 0);

    names.clear();
    nvec = 0;
}

//=============================================================================================================

void MNEProjOp::combine(const MNEProjOp& from)
{
    for (const auto& it : from.items) {
        add_item(it.vecs, it.kind, it.desc);
        items.back().active_file = it.active_file;
    }
}

//=============================================================================================================

void MNEProjOp::add_item(const MNENamedMatrix& vecs, int kind, const std::string& desc, bool is_active)
{
    items.push_back(MNEProjItem());
    auto& new_item = items.back();

    new_item.active = is_active;
    new_item.vecs = vecs;

    if (kind == FIFFV_MNE_PROJ_ITEM_EEG_AVREF) {
        new_item.has_meg = false;
        new_item.has_eeg = true;
    } else {
        for (const auto& name : vecs.collist) {
            if (name.starts_with("EEG"))
                new_item.has_eeg = true;
            if (name.starts_with("MEG"))
                new_item.has_meg = true;
        }
        if (!new_item.has_meg && !new_item.has_eeg) {
            new_item.has_meg = true;
            new_item.has_eeg = false;
        } else if (new_item.has_meg && new_item.has_eeg) {
            new_item.has_meg = true;
            new_item.has_eeg = false;
        }
    }
    if (!desc.empty())
        new_item.desc = desc;
    new_item.kind = kind;

    free_proj(); /* These data are not valid any more */
}

//=============================================================================================================

std::unique_ptr<MNEProjOp> MNEProjOp::dup() const
/*
 * Provide a duplicate (item data only)
 */
{
    auto res = std::make_unique<MNEProjOp>();

    for (const auto& it : items) {
        res->add_item(it.vecs, it.kind, it.desc, it.active);
        res->items.back().active_file = it.active_file;
    }
    return res;
}

//=============================================================================================================

std::unique_ptr<MNEProjOp> MNEProjOp::create_average_eeg_ref(std::span<const FiffChInfo> chs)
{
    StringList names;
    for (const auto& ch : chs)
        if (ch.kind == FIFFV_EEG_CH)
            names.push_back(ch.ch_name);
    const auto eegcount = static_cast<int>(names.size());
    if (eegcount == 0) {
        mne::Log::critical("No EEG channels specified for average reference.");
        return nullptr;
    }

    Eigen::MatrixXf vec_data = Eigen::MatrixXf::Constant(1, eegcount, 1.0f / sqrt(static_cast<double>(eegcount)));

    StringList emptyList;
    auto vecs = MNENamedMatrix::build(emptyList, names, vec_data);

    auto op = std::make_unique<MNEProjOp>();
    op->add_item(*vecs, FIFFV_MNE_PROJ_ITEM_EEG_AVREF, "Average EEG reference");

    return op;
}

//=============================================================================================================

int MNEProjOp::affect(std::span<const std::string> list) const
{
    int naff = 0;
    for (const auto& item : items)
        if (item.active && item.affect(list))
            naff += item.nvec();
    return naff;
}

//=============================================================================================================

int MNEProjOp::affect_chs(std::span<const FiffChInfo> chs) const
{
    StringList list;
    list.reserve(chs.size());
    for (const auto& ch : chs)
        list.push_back(ch.ch_name);
    return affect(list);
}

//=============================================================================================================

bool MNEProjOp::project_vector(Eigen::Ref<Eigen::VectorXf> vec, bool do_complement) const
{
    if (items.empty() || nvec <= 0)
        return true;

    if (names.size() != static_cast<std::size_t>(vec.size())) {
        mne::Log::critical("Data vector size does not match projection operator");
        return false;
    }

    const Eigen::VectorXf proj = proj_data.transpose() * (proj_data * vec);
    if (do_complement)
        vec -= proj;
    else
        vec = proj;
    return true;
}

//=============================================================================================================

std::unique_ptr<MNEProjOp> MNEProjOp::read_from_node(FiffStream& stream, const std::shared_ptr<FiffDirNode>& start)
/*
     * Load all the linear projection data
     */
{
    std::vector<std::shared_ptr<FiffDirNode>> proj;
    std::shared_ptr<FiffDirNode> start_node;
    std::vector<std::shared_ptr<FiffDirNode>> items;
    std::shared_ptr<FiffDirNode> node;
    int k;
    std::string item_desc;
    int global_nchan, item_nchan;
    StringList item_names;
    int item_kind;
    int item_nvec;
    int item_active;
    std::optional<FiffTag> t_pTag;

    if (!start || start->empty())
        start_node = stream.dirtree();
    else
        start_node = start;

    auto op = std::make_unique<MNEProjOp>();
    proj = start_node->dir_tree_find(FIFFB_PROJ);
    if (proj.size() == 0 || proj[0]->empty()) /* The caller must recognize an empty projection */
        return op;
    /*
        * Only the first projection block is recognized
        */
    items = proj[0]->dir_tree_find(FIFFB_PROJ_ITEM);
    if (items.size() == 0 || items[0]->empty()) /* The caller must recognize an empty projection */
        return op;
    /*
        * Get a common number of channels
        */
    node = proj[0];
    if (t_pTag = node->find_tag(stream, FIFF_NCHAN); !t_pTag)
        global_nchan = 0;
    else {
        global_nchan = *t_pTag->to_int();
    }
    /*
       * Proceess each item
       */
    for (k = 0; k < std::ssize(items); k++) {
        node = items[k];
        /*
        * Complicated procedure for getting the description
        */
        item_desc.clear();

        if (t_pTag = node->find_tag(stream, FIFF_NAME); t_pTag) {
            item_desc += t_pTag->to_string();
        }

        /*
            * Take the first line of description if it exists
            */
        if (t_pTag = node->find_tag(stream, FIFF_DESCRIPTION); t_pTag) {
            const std::string desc_tag = t_pTag->to_string();
            if (!item_desc.empty())
                item_desc += ' ';
            item_desc += desc_tag.substr(0, desc_tag.find('\n'));
        }
        /*
        * Possibility to override number of channels here
        */
        if (t_pTag = node->find_tag(stream, FIFF_NCHAN); !t_pTag) {
            item_nchan = global_nchan;
        } else {
            item_nchan = *t_pTag->to_int();
        }
        if (item_nchan <= 0) {
            mne::Log::critical("Number of channels incorrectly specified for one of the projection items.");
            return nullptr;
        }
        /*
            * Take care of the channel names
            */
        if (t_pTag = node->find_tag(stream, FIFF_PROJ_ITEM_CH_NAME_LIST); !t_pTag) {
            return nullptr;
        }

        item_names = FiffStream::split_name_list(t_pTag->to_string());

        if (static_cast<int>(item_names.size()) != item_nchan) {
            mne::Log::critical("Channel name list incorrectly specified for proj item # {}", k + 1);
            item_names.clear();
            return nullptr;
        }
        /*
            * Kind of item
            */
        if (t_pTag = node->find_tag(stream, FIFF_PROJ_ITEM_KIND); !t_pTag) {
            return nullptr;
        }
        item_kind = *t_pTag->to_int();
        /*
            * How many vectors
            */
        if (t_pTag = node->find_tag(stream, FIFF_PROJ_ITEM_NVEC); !t_pTag) {
            return nullptr;
        }
        item_nvec = *t_pTag->to_int();
        /*
            * The projection data
            */
        if (t_pTag = node->find_tag(stream, FIFF_PROJ_ITEM_VECTORS); !t_pTag) {
            return nullptr;
        }

        MatrixXf item_vectors = t_pTag->to_float_matrix().transpose();
        if (item_vectors.rows() != item_nvec || item_vectors.cols() != item_nchan) {
            mne::Log::critical("Projection item # {} stores a {} x {} matrix for {} vectors of {} channels",
                               k + 1, item_vectors.rows(), item_vectors.cols(), item_nvec, item_nchan);
            return nullptr;
        }

        /*
            * Is this item active?
            */
        if (t_pTag = node->find_tag(stream, FIFF_MNE_PROJ_ITEM_ACTIVE); t_pTag) {
            item_active = *t_pTag->to_int();
        } else
            item_active = false;
        /*
        * Ready to add
        */
        StringList emptyList;
        auto item = MNENamedMatrix::build(emptyList, item_names, item_vectors);
        op->add_item(*item, item_kind, item_desc, item_active);
        op->items.back().active_file = item_active;
    }

    return op;
}

//=============================================================================================================

std::unique_ptr<MNEProjOp> MNEProjOp::read(const std::filesystem::path& name)
{
    FiffStream stream(name);

    if (!stream.open())
        return nullptr;

    std::shared_ptr<FiffDirNode> t_default;
    auto res = read_from_node(stream, t_default);

    stream.close();

    return res;
}

//=============================================================================================================

void MNEProjOp::report_data(std::ostream& out, const std::string& tag, bool list_data, const StringList& exclude) const
/*
     * Output info about the projection operator
     */
{
    int p, q;
    const MNENamedMatrix* vecs;
    bool found;

    if (items.empty()) {
        out << "Empty operator\n";
        return;
    }

    for (std::size_t k = 0; k < items.size(); k++) {
        const auto& it = items[k];
        if (list_data && !tag.empty())
            out << tag << "\n";
        if (!tag.empty())
            out << tag;
        out << "# " << (k + 1) << " : " << it.desc << " : " << it.nvec() << " vecs : " << it.vecs.ncol() << " chs "
            << (it.has_meg ? "MEG" : "EEG") << " "
            << (it.active ? "active" : "idle") << "\n";
        if (list_data && !tag.empty())
            out << tag << "\n";
        if (list_data) {
            vecs = &items[k].vecs;

            for (q = 0; q < vecs->ncol(); q++) {
                out << std::format("{:<10}", vecs->collist[q]);
                out << (q < vecs->ncol() - 1 ? " " : "\n");
            }
            for (p = 0; p < vecs->nrow(); p++)
                for (q = 0; q < vecs->ncol(); q++) {
                    found = mne::has_channel(exclude, vecs->collist[q]);
                    out << std::format("{:10.5g} ", found ? 0.0 : static_cast<double>(vecs->data(p, q)));
                    out << (q < vecs->ncol() - 1 ? " " : "\n");
                }
            if (list_data && !tag.empty())
                out << tag << "\n";
        }
    }
    return;
}

//=============================================================================================================

void MNEProjOp::report(std::ostream& out, const std::string& tag) const
{
    report_data(out, tag, false, StringList());
}

//=============================================================================================================

void MNEProjOp::assign_channels(std::span<const std::string> list)
{
    free_proj(); /* Compiled data is no longer valid */
    names.assign(list.begin(), list.end());
}

//=============================================================================================================

namespace
{

void clear_channel_group(Eigen::Ref<Eigen::VectorXf> data, const StringList& ch_names, std::string_view prefix)
{
    for (std::size_t k = 0; k < ch_names.size(); k++)
        if (ch_names[k].starts_with(prefix))
            data[static_cast<Eigen::Index>(k)] = 0.0;
}

constexpr float USE_LIMIT = 1e-5f;
constexpr float SMALL_VALUE = 1e-4f;

} // anonymous namespace

bool MNEProjOp::make_proj(const StringList& bad)
{
    using RowMatrixXf = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

    int k, p, q, r, nvec_total;
    RowMatrixXf vv_meg_mat;
    Eigen::VectorXf sing_meg_vec;
    RowMatrixXf vv_eeg_mat;
    Eigen::VectorXf sing_eeg_vec;
    int nvec_meg;
    int nvec_eeg;
    MNENamedVector vec;
    float size;
    int nzero;

    proj_data.resize(0, 0);
    nvec = 0;

    const auto nch = static_cast<int>(names.size());
    if (nch == 0 || items.empty())
        return true;

    nvec_total = affect(names);
    if (nvec_total == 0)
        return true;

    RowMatrixXf mat_meg_mat = RowMatrixXf::Zero(nvec_total, nch);
    RowMatrixXf mat_eeg_mat = RowMatrixXf::Zero(nvec_total, nch);

    for (k = 0, nvec_meg = nvec_eeg = 0; k < static_cast<int>(items.size()); k++) {
        if (items[k].active && items[k].affect(names)) {
            vec.names = items[k].vecs.collist;
            if (items[k].has_meg) {
                for (p = 0; p < items[k].nvec(); p++, nvec_meg++) {
                    vec.data = items[k].vecs.data.row(p);
                    Eigen::Map<Eigen::VectorXf> res_meg(mat_meg_mat.row(nvec_meg).data(), nch);
                    if (!vec.pick(names, false, res_meg))
                        return false;
                }
            } else if (items[k].has_eeg) {
                for (p = 0; p < items[k].nvec(); p++, nvec_eeg++) {
                    vec.data = items[k].vecs.data.row(p);
                    Eigen::Map<Eigen::VectorXf> res_eeg(mat_eeg_mat.row(nvec_eeg).data(), nch);
                    if (!vec.pick(names, false, res_eeg))
                        return false;
                }
            }
        }
    }
    /*
     * Replace bad channel entries with zeroes
     */
    for (q = 0; q < static_cast<int>(bad.size()); q++)
        for (r = 0; r < nch; r++)
            if (names[r] == bad[q]) {
                for (p = 0; p < nvec_meg; p++)
                    mat_meg_mat(p, r) = 0.0;
                for (p = 0; p < nvec_eeg; p++)
                    mat_eeg_mat(p, r) = 0.0;
            }
    /*
     * Scale the rows so that detection of linear dependence becomes easy
     */
    for (p = 0, nzero = 0; p < nvec_meg; p++) {
        size = mat_meg_mat.row(p).norm();
        if (size > 0) {
            mat_meg_mat.row(p) /= size;
        } else
            nzero++;
    }
    if (nzero == nvec_meg) {
        mat_meg_mat.resize(0, 0);
        nvec_meg = 0;
    }
    for (p = 0, nzero = 0; p < nvec_eeg; p++) {
        size = mat_eeg_mat.row(p).norm();
        if (size > 0) {
            mat_eeg_mat.row(p) /= size;
        } else
            nzero++;
    }
    if (nzero == nvec_eeg) {
        mat_eeg_mat.resize(0, 0);
        nvec_eeg = 0;
    }
    if (nvec_meg + nvec_eeg == 0) {
        mne::Log::warning("No projection remains after excluding bad channels. Omitting projection.");
        return true;
    }
    /*
     * Proceed to SVD
     */
    if (nvec_meg > 0) {
        Eigen::JacobiSVD<Eigen::MatrixXf> svd(mat_meg_mat.topRows(nvec_meg), Eigen::ComputeFullV);
        sing_meg_vec = svd.singularValues();
        vv_meg_mat = svd.matrixV().transpose().topRows(nvec_meg);
    }
    if (nvec_eeg > 0) {
        Eigen::JacobiSVD<Eigen::MatrixXf> svd(mat_eeg_mat.topRows(nvec_eeg), Eigen::ComputeFullV);
        sing_eeg_vec = svd.singularValues();
        vv_eeg_mat = svd.matrixV().transpose().topRows(nvec_eeg);
    }
    /*
     * Check for linearly dependent vectors
     */
    for (p = 0, nvec = 0; p < nvec_meg; p++, nvec++)
        if (sing_meg_vec[p] / sing_meg_vec[0] < USE_LIMIT)
            break;
    for (p = 0; p < nvec_eeg; p++, nvec++)
        if (sing_eeg_vec[p] / sing_eeg_vec[0] < USE_LIMIT)
            break;
    proj_data = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>::Zero(nvec, nch);
    for (p = 0, nvec = 0; p < nvec_meg; p++, nvec++) {
        if (sing_meg_vec[p] / sing_meg_vec[0] < USE_LIMIT)
            break;
        for (k = 0; k < nch; k++) {
            if (std::fabs(vv_meg_mat(p, k)) < SMALL_VALUE)
                proj_data(nvec, k) = 0.0;
            else
                proj_data(nvec, k) = vv_meg_mat(p, k);
        }
        clear_channel_group(Eigen::Map<Eigen::VectorXf>(proj_data.row(nvec).data(), nch), names, "EEG");
    }
    for (p = 0; p < nvec_eeg; p++, nvec++) {
        if (sing_eeg_vec[p] / sing_eeg_vec[0] < USE_LIMIT)
            break;
        for (k = 0; k < nch; k++) {
            if (std::fabs(vv_eeg_mat(p, k)) < SMALL_VALUE)
                proj_data(nvec, k) = 0.0;
            else
                proj_data(nvec, k) = vv_eeg_mat(p, k);
        }
        clear_channel_group(Eigen::Map<Eigen::VectorXf>(proj_data.row(nvec).data(), nch), names, "MEG");
    }
    /*
     * Make sure that the stimulus channels are not modified
     */
    for (k = 0; k < nch; k++)
        if (names[k].starts_with("STI")) {
            for (p = 0; p < nvec; p++)
                proj_data(p, k) = 0.0;
        }

    return true;
}

//=============================================================================================================

bool MNEProjOp::project_dvector(Eigen::Ref<Eigen::VectorXd> vec, bool do_complement) const
{
    if (nvec <= 0)
        return true;

    if (names.size() != static_cast<std::size_t>(vec.size())) {
        mne::Log::critical("Data vector size does not match projection operator");
        return false;
    }

    const Eigen::MatrixXd u = proj_data.cast<double>();
    const Eigen::VectorXd proj = u.transpose() * (u * vec);
    if (do_complement)
        vec -= proj;
    else
        vec = proj;
    return true;
}

//=============================================================================================================

mne::Expected<std::unique_ptr<MNEProjOp>> MNEProjOp::make_projection(const std::vector<std::filesystem::path>& projnames,
                                                                     std::span<const FiffChInfo> chs)
{
    const bool has_eeg = std::ranges::any_of(chs, [](const FiffChInfo& ch) { return ch.kind == FIFFV_EEG_CH; });

    if (projnames.empty() && !has_eeg)
        return nullptr;

    std::unique_ptr<MNEProjOp> all;

    for (const auto& projname : projnames) {
        std::unique_ptr<MNEProjOp> one(MNEProjOp::read(projname));
        if (!one)
            return mne::fail(mne::ErrorCode::Io, "cannot read the projection in {}", mne::utf8_from_path(projname));
        if (one->items.empty()) {
            mne::Log::info("No linear projection information in {}.", mne::utf8_from_path(projname));
        } else {
            mne::Log::info("Loaded projection from {}:", mne::utf8_from_path(projname));
            {
                std::ostringstream report;
                one->report(report, "\t");
                mne::Log::info("{}", report.str());
            }
            if (!all)
                all = std::make_unique<MNEProjOp>();
            all->combine(*one);
        }
    }

    if (has_eeg) {
        const bool found = all && std::ranges::any_of(all->items, [](const MNEProjItem& it) { return it.kind == FIFFV_MNE_PROJ_ITEM_EEG_AVREF; });
        if (!found) {
            std::unique_ptr<MNEProjOp> one(MNEProjOp::create_average_eeg_ref(chs));
            if (one) {
                mne::Log::info("Average EEG reference projection added:");
                {
                    std::ostringstream report;
                    one->report(report, "\t");
                    mne::Log::info("{}", report.str());
                }
                if (!all)
                    all = std::make_unique<MNEProjOp>();
                all->combine(*one);
            }
        }
    }
    if (all && all->affect_chs(chs) == 0) {
        mne::Log::info("Projection will not have any effect on selected channels. Projection omitted.");
        all.reset();
    }
    return all;
}

//=============================================================================================================

bool MNEProjOp::apply_cov(MNECovMatrix& c) const
{
    using RowMatrixXd = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

    int j, k, p;
    bool do_complement = true;

    if (items.empty())
        return true;

    if (static_cast<int>(names.size()) != c.ncov || names != c.names) {
        mne::Log::critical("Incompatible data in apply_cov");
        return false;
    }

    RowMatrixXd dcovMat = RowMatrixXd::Zero(c.ncov, c.ncov);

    if (c.cov_diag.size() > 0) {
        for (j = 0, p = 0; j < c.ncov; j++)
            for (k = 0; k < c.ncov; k++)
                dcovMat(j, k) = (j == k) ? c.cov_diag[j] : 0;
    } else {
        for (j = 0, p = 0; j < c.ncov; j++)
            for (k = 0; k <= j; k++)
                dcovMat(j, k) = c.cov[p++];
        for (j = 0; j < c.ncov; j++)
            for (k = j + 1; k < c.ncov; k++)
                dcovMat(j, k) = dcovMat(k, j);
    }

    for (k = 0; k < c.ncov; k++) {
        Eigen::Map<Eigen::VectorXd> row_k(dcovMat.row(k).data(), c.ncov);
        if (!project_dvector(row_k, do_complement))
            return false;
    }

    dcovMat.transposeInPlace();

    for (k = 0; k < c.ncov; k++) {
        Eigen::Map<Eigen::VectorXd> row_k(dcovMat.row(k).data(), c.ncov);
        if (!project_dvector(row_k, do_complement))
            return false;
    }

    if (c.cov_diag.size() > 0) {
        for (j = 0; j < c.ncov; j++) {
            c.cov_diag[j] = dcovMat(j, j);
        }
        c.cov.resize(0);
    } else {
        for (j = 0, p = 0; j < c.ncov; j++)
            for (k = 0; k <= j; k++)
                c.cov[p++] = dcovMat(j, k);
    }

    c.nproj = affect(c.names);
    return true;
}
