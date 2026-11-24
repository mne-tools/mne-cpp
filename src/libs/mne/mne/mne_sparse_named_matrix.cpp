//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_sparse_named_matrix.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Implementation of @ref mne::MNESparseNamedMatrix (MNE-C mne_named_matrix.c).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_sparse_named_matrix.h"

#include <mne/base/log.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_tag.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::unique_ptr<MNESparseNamedMatrix> MNESparseNamedMatrix::read(FiffStream& stream,
                                                                 const std::shared_ptr<FiffDirNode>& node,
                                                                 int kind)
{
    std::shared_ptr<FiffDirNode> block;
    std::optional<FiffTag> tag;
    if (node->type == FIFFB_MNE_NAMED_MATRIX) {
        if (tag = node->find_tag(stream, kind); tag) {
            block = node;
        }
    } else {
        for (const std::shared_ptr<FiffDirNode>& child : node->children) {
            if (child->type != FIFFB_MNE_NAMED_MATRIX) {
                continue;
            }
            if (tag = child->find_tag(stream, kind); tag) {
                block = child;
                break;
            }
        }
    }
    if (!block) {
        return nullptr;
    }
    std::unique_ptr<FiffSparseMatrix> data = FiffSparseMatrix::fiff_get_float_sparse_matrix(*tag);
    if (!data) {
        return nullptr;
    }
    auto matrix = std::make_unique<MNESparseNamedMatrix>();
    matrix->data = std::move(*data);
    if (const auto nrow = block->find_tag(stream, FIFF_MNE_NROW); nrow && *nrow->to_int() != matrix->nrow()) {
        mne::Log::critical("MNESparseNamedMatrix::read - FIFF_MNE_NROW conflicts with the matrix data.");
        return nullptr;
    }
    if (const auto ncol = block->find_tag(stream, FIFF_MNE_NCOL); ncol && *ncol->to_int() != matrix->ncol()) {
        mne::Log::critical("MNESparseNamedMatrix::read - FIFF_MNE_NCOL conflicts with the matrix data.");
        return nullptr;
    }
    if (tag = block->find_tag(stream, FIFF_MNE_ROW_NAMES); tag) {
        matrix->rowlist = FiffStream::split_name_list(tag->to_string());
        if (static_cast<int>(matrix->rowlist.size()) != matrix->nrow()) {
            mne::Log::critical("MNESparseNamedMatrix::read - Incorrect number of entries in the row name list.");
            return nullptr;
        }
    }
    if (tag = block->find_tag(stream, FIFF_MNE_COL_NAMES); tag) {
        matrix->collist = FiffStream::split_name_list(tag->to_string());
        if (static_cast<int>(matrix->collist.size()) != matrix->ncol()) {
            mne::Log::critical("MNESparseNamedMatrix::read - Incorrect number of entries in the column name list.");
            return nullptr;
        }
    }
    return matrix;
}

//=============================================================================================================

void MNESparseNamedMatrix::write(FiffStream& stream, int kind) const
{
    stream.start_block(FIFFB_MNE_NAMED_MATRIX);
    const fiff_int_t rows = nrow();
    const fiff_int_t cols = ncol();
    stream.write_int(FIFF_MNE_NROW, &rows);
    stream.write_int(FIFF_MNE_NCOL, &cols);
    if (!rowlist.empty()) {
        stream.write_name_list(FIFF_MNE_ROW_NAMES, rowlist);
    }
    if (!collist.empty()) {
        stream.write_name_list(FIFF_MNE_COL_NAMES, collist);
    }
    stream.write_float_sparse_rcs(kind, data.eigen());
    stream.end_block(FIFFB_MNE_NAMED_MATRIX);
}
