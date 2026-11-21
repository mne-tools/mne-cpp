//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_named_matrix.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of @ref mne::MNENamedMatrix.
 *
 * Implements FIFF read/write of the @c FIFF_MNE_ROW_NAMES,
 * @c FIFF_MNE_COL_NAMES and @c FIFF_MNE_NAMED_MATRIX tags, plus the
 * channel-name based row/column lookup used by @ref MNEProjOp and
 * @ref MNECTFCompDataSet.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_named_matrix.h"

#include <base/log.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <memory>
#include <algorithm>
#include <iterator>
#include <numeric>
#include <optional>
#include <span>
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

std::unique_ptr<MNENamedMatrix> MNENamedMatrix::build(const StringList& rowlist,
                                                      const StringList& collist,
                                                      const Eigen::MatrixXf& data)
{
    if ((!rowlist.empty() && std::ssize(rowlist) != data.rows()) || (!collist.empty() && std::ssize(collist) != data.cols())) {
        mne::Log::critical("MNENamedMatrix::build - {} row and {} column names do not fit a {} x {} matrix.",
                           rowlist.size(), collist.size(), data.rows(), data.cols());
        return nullptr;
    }
    auto mat = std::make_unique<MNENamedMatrix>();
    mat->rowlist = rowlist;
    mat->collist = collist;
    mat->data = data;
    return mat;
}

//=============================================================================================================

namespace
{

// Indices of the picked names in the original list; all of them when nothing is picked.
std::optional<std::vector<Eigen::Index>> pick_indices(std::span<const std::string> picks,
                                                      const StringList& names,
                                                      Eigen::Index count,
                                                      std::string_view what)
{
    std::vector<Eigen::Index> indices;
    if (picks.empty()) {
        indices.resize(static_cast<std::size_t>(count));
        std::iota(indices.begin(), indices.end(), Eigen::Index{0});
        return indices;
    }
    if (names.empty()) {
        mne::Log::critical("MNENamedMatrix::pick - Cannot pick {}s: no {} names in original matrix.", what, what);
        return std::nullopt;
    }
    indices.reserve(picks.size());
    for (const auto& name : picks) {
        const auto it = std::ranges::find(names, name);
        if (it == names.end()) {
            mne::Log::critical("MNENamedMatrix::pick - {} '{}' not found in original matrix.", what, name);
            return std::nullopt;
        }
        indices.push_back(std::distance(names.begin(), it));
    }
    return indices;
}

} // anonymous namespace

std::unique_ptr<MNENamedMatrix> MNENamedMatrix::pick(std::span<const std::string> pickrowlist,
                                                     std::span<const std::string> pickcollist) const
{
    const auto rows = pick_indices(pickrowlist, rowlist, data.rows(), "row");
    const auto cols = pick_indices(pickcollist, collist, data.cols(), "column");
    if (!rows || !cols)
        return nullptr;

    auto mat = std::make_unique<MNENamedMatrix>();
    mat->rowlist = pickrowlist.empty() ? rowlist : StringList(pickrowlist.begin(), pickrowlist.end());
    mat->collist = pickcollist.empty() ? collist : StringList(pickcollist.begin(), pickcollist.end());
    mat->data = data(*rows, *cols);
    return mat;
}

//=============================================================================================================

std::unique_ptr<MNENamedMatrix> MNENamedMatrix::read(FiffStream& stream,
                                                     const std::shared_ptr<FiffDirNode>& node,
                                                     int kind)
{
    StringList colnames;
    StringList rownames;
    int ncol = 0;
    int nrow = 0;
    std::int32_t ndim;
    std::vector<std::int32_t> dims;
    MatrixXf data;
    std::optional<FiffTag> t_pTag;
    bool dataFound = false;

    std::shared_ptr<FiffDirNode> tmp_node = node;

    /*
     * If the node itself is a FIFFB_MNE_NAMED_MATRIX block, read from it.
     * Otherwise look in its first-generation children for such a block.
     */
    if (tmp_node->type == FIFFB_MNE_NAMED_MATRIX) {
        if (t_pTag = tmp_node->find_tag(stream, kind); !t_pTag)
            return nullptr;

        t_pTag->matrix_dimensions(ndim, dims);
        if (ndim != 2) {
            mne::Log::critical("MNENamedMatrix::read - Only two-dimensional matrices are supported.");
            return nullptr;
        }

        data = t_pTag->to_float_matrix().transpose();
        dataFound = true;
    } else {
        for (int k = 0; k < tmp_node->nchild(); ++k) {
            if (tmp_node->children[k]->type == FIFFB_MNE_NAMED_MATRIX) {
                if (t_pTag = tmp_node->children[k]->find_tag(stream, kind); t_pTag) {
                    t_pTag->matrix_dimensions(ndim, dims);
                    if (ndim != 2) {
                        mne::Log::critical("MNENamedMatrix::read - Only two-dimensional matrices are supported.");
                        return nullptr;
                    }

                    data = t_pTag->to_float_matrix().transpose();
                    dataFound = true;
                    tmp_node = tmp_node->children[k];
                    break;
                }
            }
        }
        if (!dataFound)
            return nullptr;
    }

    /*
     * Read optional FIFF_MNE_NROW / FIFF_MNE_NCOL dimension tags and
     * cross-check them against the matrix data.
     */
    if (t_pTag = tmp_node->find_tag(stream, FIFF_MNE_NROW); !t_pTag) {
        nrow = static_cast<int>(data.rows());
    } else {
        nrow = *t_pTag->to_int();
        if (nrow != data.rows()) {
            mne::Log::critical("MNENamedMatrix::read - FIFF_MNE_NROW tag ({}) conflicts with matrix data ({}).", nrow, data.rows());
            return nullptr;
        }
    }

    if (t_pTag = tmp_node->find_tag(stream, FIFF_MNE_NCOL); !t_pTag) {
        ncol = static_cast<int>(data.cols());
    } else {
        ncol = *t_pTag->to_int();
        if (ncol != data.cols()) {
            mne::Log::critical("MNENamedMatrix::read - FIFF_MNE_NCOL tag ({}) conflicts with matrix data ({}).", ncol, data.cols());
            return nullptr;
        }
    }

    /*
     * Read optional row and column name lists.
     */
    if (t_pTag = tmp_node->find_tag(stream, FIFF_MNE_ROW_NAMES); t_pTag) {
        const std::string s = t_pTag->to_string();
        rownames = FiffStream::split_name_list(s);
        if (static_cast<int>(rownames.size()) != nrow) {
            mne::Log::critical("MNENamedMatrix::read - Row name count ({}) does not match nrow ({}).", rownames.size(), nrow);
            return nullptr;
        }
    }

    if (t_pTag = tmp_node->find_tag(stream, FIFF_MNE_COL_NAMES); t_pTag) {
        const std::string s = t_pTag->to_string();
        colnames = FiffStream::split_name_list(s);
        if (static_cast<int>(colnames.size()) != ncol) {
            mne::Log::critical("MNENamedMatrix::read - Column name count ({}) does not match ncol ({}).", colnames.size(), ncol);
            return nullptr;
        }
    }

    return build(rownames, colnames, data);
}
