//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_sparse_named_matrix.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Sparse variant of @ref mne::MNENamedMatrix, i.e. a sparse matrix with row and column name lists.
 *
 * Stored in a @c FIFFB_MNE_NAMED_MATRIX block like its dense counterpart;
 * MNE-C uses it for channel derivations (@ref mne::MNEDerivSet).
 */

#ifndef MNE_SPARSE_NAMED_MATRIX_H
#define MNE_SPARSE_NAMED_MATRIX_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include <mne/fiff/fiff_dir_node.h>
#include <mne/fiff/fiff_sparse_matrix.h>
#include <mne/fiff/fiff_stream.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>
#include <vector>
#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
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
    static std::unique_ptr<MNESparseNamedMatrix> read(mne::FiffStream& stream,
                                                      const std::shared_ptr<mne::FiffDirNode>& node,
                                                      int kind);

    //=========================================================================================================
    /**
     * Writes the matrix as a @c FIFFB_MNE_NAMED_MATRIX block (MNE-C @c mne_write_sparse_named_matrix).
     *
     * @param[in] stream   A FIFF stream open for writing.
     * @param[in] kind     The tag for the sparse matrix.
     */
    void write(mne::FiffStream& stream, int kind) const;

    //=========================================================================================================
    /**
     * @return The number of rows of @ref data.
     */
    int nrow() const
    {
        return data.rows();
    }

    //=========================================================================================================
    /**
     * @return The number of columns of @ref data.
     */
    int ncol() const
    {
        return data.cols();
    }

    StringList rowlist;         /**< Name list for the rows. */
    StringList collist;         /**< Name list for the columns. */
    mne::FiffSparseMatrix data; /**< The data itself (sparse). */
};

} // namespace mne

#endif // MNE_SPARSE_NAMED_MATRIX_H
