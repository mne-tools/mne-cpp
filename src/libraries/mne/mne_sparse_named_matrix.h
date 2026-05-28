//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_sparse_named_matrix.h
 * @since March 2026
 * @brief Sparse variant of @ref MNELIB::MNENamedMatrix backed by an Eigen @c SparseMatrix.
 *
 * Used by inverse operators and clustering machinery where the gain or
 * projection matrix is dominated by zero columns (clustered leadfield,
 * label-restricted projections). Preserves the row/column name lists of
 * the dense flavour so the same channel-name lookups continue to work.
 */

#ifndef MNE_SPARSE_NAMED_MATRIX_H
#define MNE_SPARSE_NAMED_MATRIX_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

#include <fiff/fiff_sparse_matrix.h>

#include <QStringList>

#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

/**
 * Sparse named matrix - matrix specification with row/column name lists.
 */
class MNESHARED_EXPORT MNESparseNamedMatrix
{
public:
    MNESparseNamedMatrix() = default;
    ~MNESparseNamedMatrix() = default;

    int   nrow = 0;                        /**< Number of rows (same as in data). */
    int   ncol = 0;                        /**< Number of columns (same as in data). */
    QStringList rowlist;                   /**< Name list for the rows. */
    QStringList collist;                   /**< Name list for the columns. */
    std::unique_ptr<FIFFLIB::FiffSparseMatrix> data; /**< The data itself (sparse). */
};

} // namespace MNELIB

#endif // MNE_SPARSE_NAMED_MATRIX_H
