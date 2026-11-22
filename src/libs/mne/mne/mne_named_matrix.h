//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     mne_named_matrix.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     January 2017
 * @brief    Row/column-labelled dense matrix used wherever FIFF stores per-channel data.
 *
 * @ref mne::MNENamedMatrix is the C++ port of @c fiff_named_matrix:
 * an Eigen dense matrix carrying explicit @c StringList row and column
 * names. Leadfields, SSP vectors, CTF compensators and many derived
 * matrices are stored in this form so callers can re-order or subset by
 * channel name without depending on positional indices.
 */

#ifndef MNENAMEDMATRIX_H
#define MNENAMEDMATRIX_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>
#include <vector>
#include <memory>
#include <span>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class FiffStream;
class FiffDirNode;
}

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief A dense matrix with named rows and columns.
 *
 * MNENamedMatrix associates a dense float matrix (Eigen::MatrixXf) with optional
 * row and column name lists (StringList). It is the C++ equivalent of the MNE-C
 * @c mneNamedMatrixRec struct, and is used throughout the library to represent
 * projection vectors, CTF compensation matrices, and forward-solution gain matrices.
 *
 * The class provides factory methods to:
 * - build() a matrix from its constituent parts,
 * - read() a matrix from a FIFF stream, and
 * - pick() a sub-matrix by selecting named rows and/or columns.
 *
 * All factory methods return @c std::unique_ptr<MNENamedMatrix> for clear ownership.
 *
 * @note This class is functionally similar to mne::FiffNamedMatrix.
 *       A future consolidation of both types is planned.
 *
 * @snippet ex_mne_api/main.cpp mne_ctf_comp_data_set_usage
 */
class MNESHARED_EXPORT MNENamedMatrix
{
public:
    //=========================================================================================================
    /**
     * @brief Factory: build a named matrix from its constituent parts.
     *
     * @param[in] rowlist    Row names: empty, or one per row of @p data.
     * @param[in] collist    Column names: empty, or one per column of @p data.
     * @param[in] data       Dense float matrix to store.
     *
     * @return The named matrix, or @c nullptr if a name list does not fit @p data.
     */
    static std::unique_ptr<MNENamedMatrix> build(const StringList& rowlist,
                                                 const StringList& collist,
                                                 const Eigen::MatrixXf& data);

    //=========================================================================================================
    /**
     * @brief Factory: read a named matrix from a FIFF file.
     *
     * Reads a two-dimensional tagged matrix of the given @p kind from
     * @p node (or its first FIFFB_MNE_NAMED_MATRIX child). Row and column
     * name lists are read from the associated FIFF_MNE_ROW_NAMES and
     * FIFF_MNE_COL_NAMES tags when present.
     *
     * @param[in] stream     Open FIFF stream to read from.
     * @param[in] node       Directory node that contains (or is) the named-matrix block.
     * @param[in] kind       FIFF tag kind that identifies the matrix data (e.g. FIFF_MNE_FORWARD_SOLUTION).
     *
     * @return A unique pointer to the read matrix, or @c nullptr on failure.
     */
    static std::unique_ptr<MNENamedMatrix> read(mne::FiffStream& stream,
                                                const std::shared_ptr<mne::FiffDirNode>& node,
                                                int kind);

    //=========================================================================================================
    /**
     * @brief Create a sub-matrix by picking named rows and columns.
     *
     * Selects a subset of this matrix's rows and columns by matching their
     * names against the supplied pick-lists. If a pick-list is empty, all
     * rows (or columns) of the original are retained.
     *
     * @param[in] pickrowlist    Row names to select (empty = keep all rows).
     * @param[in] pickcollist    Column names to select (empty = keep all columns).
     *
     * @return The picked sub-matrix, or @c nullptr if a requested name was not found.
     */
    std::unique_ptr<MNENamedMatrix> pick(std::span<const std::string> pickrowlist,
                                         std::span<const std::string> pickcollist) const;

    //=========================================================================================================
    /**
     * @return The number of rows of @ref data.
     */
    int nrow() const
    {
        return static_cast<int>(data.rows());
    }

    //=========================================================================================================
    /**
     * @return The number of columns of @ref data.
     */
    int ncol() const
    {
        return static_cast<int>(data.cols());
    }

public:
    StringList rowlist;   /**< Name list for the rows (may be empty if unnamed). */
    StringList collist;   /**< Name list for the columns (may be empty if unnamed). */
    Eigen::MatrixXf data; /**< Dense data matrix. */
};

} // NAMESPACE mne_mne

#endif // MNENAMEDMATRIX_H
