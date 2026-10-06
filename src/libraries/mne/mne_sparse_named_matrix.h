//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_sparse_named_matrix.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Sparse variant of @ref MNELIB::MNENamedMatrix, i.e. a sparse matrix with row and column name lists.
 *
 * Stored in a @c FIFFB_MNE_NAMED_MATRIX block like its dense counterpart;
 * MNE-C uses it for channel derivations (@ref MNELIB::MNEDerivSet).
 */

#ifndef MNE_SPARSE_NAMED_MATRIX_H
#define MNE_SPARSE_NAMED_MATRIX_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

#include <fiff/fiff_dir_node.h>
#include <fiff/fiff_sparse_matrix.h>
#include <fiff/fiff_stream.h>

#include <QStringList>

#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

/**
 * Sparse named matrix - matrix specification with row/column name lists.
 *
 * @snippet ex_mne_api/main.cpp mne_deriv_set_usage
 */
class MNESHARED_EXPORT MNESparseNamedMatrix
{
public:
    MNESparseNamedMatrix() = default;
    ~MNESparseNamedMatrix() = default;

    //=========================================================================================================
    /**
     * Reads a sparse named matrix (MNE-C @c mne_read_sparse_named_matrix).
     *
     * @param[in] stream   An open FIFF stream.
     * @param[in] node     A @c FIFFB_MNE_NAMED_MATRIX block, or a block whose direct children contain one.
     * @param[in] kind     The tag holding the sparse matrix.
     *
     * @return The matrix, or nullptr if it is missing or its name lists do not fit.
     */
    static std::unique_ptr<MNESparseNamedMatrix> read(FIFFLIB::FiffStream::SPtr& stream,
                                                      const FIFFLIB::FiffDirNode::SPtr& node,
                                                      int kind);

    //=========================================================================================================
    /**
     * Writes the matrix as a @c FIFFB_MNE_NAMED_MATRIX block (MNE-C @c mne_write_sparse_named_matrix).
     *
     * @param[in] stream   A FIFF stream open for writing.
     * @param[in] kind     The tag for the sparse matrix.
     */
    void write(FIFFLIB::FiffStream& stream, int kind) const;

    int nrow = 0;                                    /**< Number of rows (same as in data). */
    int ncol = 0;                                    /**< Number of columns (same as in data). */
    QStringList rowlist;                             /**< Name list for the rows. */
    QStringList collist;                             /**< Name list for the columns. */
    std::unique_ptr<FIFFLIB::FiffSparseMatrix> data; /**< The data itself (sparse). */
};

} // namespace MNELIB

#endif // MNE_SPARSE_NAMED_MATRIX_H
