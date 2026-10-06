//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_sparse_named_matrix.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Implementation of @ref MNELIB::MNESparseNamedMatrix (MNE-C mne_named_matrix.c).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_sparse_named_matrix.h"

#include <fiff/fiff_constants.h>
#include <fiff/fiff_tag.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::unique_ptr<MNESparseNamedMatrix> MNESparseNamedMatrix::read(FiffStream::SPtr& stream,
                                                                 const FiffDirNode::SPtr& node,
                                                                 int kind)
{
    FiffDirNode::SPtr block;
    FiffTag::UPtr tag;
    if (node->type == FIFFB_MNE_NAMED_MATRIX) {
        if (node->find_tag(stream, kind, tag)) {
            block = node;
        }
    } else {
        for (const FiffDirNode::SPtr& child : node->children) {
            if (child->type == FIFFB_MNE_NAMED_MATRIX && child->find_tag(stream, kind, tag)) {
                block = child;
                break;
            }
        }
    }
    if (!block) {
        return nullptr;
    }
    FiffSparseMatrix::UPtr data = FiffSparseMatrix::fiff_get_float_sparse_matrix(tag);
    if (!data) {
        return nullptr;
    }
    auto matrix = std::make_unique<MNESparseNamedMatrix>();
    matrix->nrow = data->rows();
    matrix->ncol = data->cols();
    if (block->find_tag(stream, FIFF_MNE_NROW, tag) && *tag->toInt() != matrix->nrow) {
        qCritical("MNESparseNamedMatrix::read - FIFF_MNE_NROW conflicts with the matrix data.");
        return nullptr;
    }
    if (block->find_tag(stream, FIFF_MNE_NCOL, tag) && *tag->toInt() != matrix->ncol) {
        qCritical("MNESparseNamedMatrix::read - FIFF_MNE_NCOL conflicts with the matrix data.");
        return nullptr;
    }
    if (block->find_tag(stream, FIFF_MNE_ROW_NAMES, tag)) {
        matrix->rowlist = FiffStream::split_name_list(tag->toString());
        if (matrix->rowlist.size() != matrix->nrow) {
            qCritical("MNESparseNamedMatrix::read - Incorrect number of entries in the row name list.");
            return nullptr;
        }
    }
    if (block->find_tag(stream, FIFF_MNE_COL_NAMES, tag)) {
        matrix->collist = FiffStream::split_name_list(tag->toString());
        if (matrix->collist.size() != matrix->ncol) {
            qCritical("MNESparseNamedMatrix::read - Incorrect number of entries in the column name list.");
            return nullptr;
        }
    }
    matrix->data = std::move(data);
    return matrix;
}

//=============================================================================================================

void MNESparseNamedMatrix::write(FiffStream& stream, int kind) const
{
    stream.start_block(FIFFB_MNE_NAMED_MATRIX);
    stream.write_int(FIFF_MNE_NROW, &nrow);
    stream.write_int(FIFF_MNE_NCOL, &ncol);
    if (!rowlist.isEmpty()) {
        stream.write_name_list(FIFF_MNE_ROW_NAMES, rowlist);
    }
    if (!collist.isEmpty()) {
        stream.write_name_list(FIFF_MNE_COL_NAMES, collist);
    }
    stream.write_float_sparse_rcs(kind, data->eigen());
    stream.end_block(FIFFB_MNE_NAMED_MATRIX);
}
