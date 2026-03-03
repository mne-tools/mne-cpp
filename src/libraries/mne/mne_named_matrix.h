//=============================================================================================================
/**
 * @file     mne_named_matrix.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    MneNamedMatrix class declaration.
 *
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

#include <memory>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QStringList>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB
{
    class FiffStream;
    class FiffDirNode;
}

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
/**
 * @brief A dense matrix with named rows and columns.
 *
 * MneNamedMatrix associates a dense float matrix (Eigen::MatrixXf) with optional
 * row and column name lists (QStringList). It is the C++ equivalent of the MNE-C
 * @c mneNamedMatrixRec struct, and is used throughout the library to represent
 * projection vectors, CTF compensation matrices, and forward-solution gain matrices.
 *
 * The class provides factory methods to:
 * - build() a matrix from its constituent parts,
 * - read() a matrix from a FIFF stream, and
 * - pick() a sub-matrix by selecting named rows and/or columns.
 *
 * All factory methods return @c std::unique_ptr<MneNamedMatrix> for clear ownership.
 *
 * @note This class is functionally similar to FIFFLIB::FiffNamedMatrix.
 *       A future consolidation of both types is planned.
 */
class MNESHARED_EXPORT MneNamedMatrix
{
public:
    typedef QSharedPointer<MneNamedMatrix> SPtr;              /**< Shared pointer type for MneNamedMatrix. */
    typedef QSharedPointer<const MneNamedMatrix> ConstSPtr;   /**< Const shared pointer type for MneNamedMatrix. */

    //=========================================================================================================
    /**
     * @brief Default constructor.
     *
     * Creates an empty named matrix with zero rows and columns
     * and no associated name lists.
     */
    MneNamedMatrix();

    //=========================================================================================================
    /**
     * @brief Copy constructor.
     *
     * Performs a deep copy of all members: dimensions, name lists, and matrix data.
     *
     * @param[in] p_MneNamedMatrix   The named matrix to copy.
     */
    MneNamedMatrix(const MneNamedMatrix& p_MneNamedMatrix);

    //=========================================================================================================
    /**
     * @brief Destructor.
     *
     * All members (Eigen::MatrixXf, QStringList) are value types and clean
     * themselves up automatically.
     */
    ~MneNamedMatrix();

    //=========================================================================================================
    /**
     * @brief Factory: build a named matrix from its constituent parts.
     *
     * Assembles an MneNamedMatrix from dimension counts, optional row/column
     * name lists, and a dense data matrix.
     *
     * @param[in] nrow       Number of rows (must match @p data.rows()).
     * @param[in] ncol       Number of columns (must match @p data.cols()).
     * @param[in] rowlist    Name list for the rows (may be empty).
     * @param[in] collist    Name list for the columns (may be empty).
     * @param[in] data       Dense float matrix to store.
     *
     * @return A unique pointer to the newly created named matrix.
     */
    static std::unique_ptr<MneNamedMatrix> build(int nrow,
                                                 int ncol,
                                                 const QStringList& rowlist,
                                                 const QStringList& collist,
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
    static std::unique_ptr<MneNamedMatrix> read(QSharedPointer<FIFFLIB::FiffStream>& stream,
                                                const QSharedPointer<FIFFLIB::FiffDirNode>& node,
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
     * @param[in] picknrow       Expected number of picked rows (used when @p pickrowlist is empty).
     * @param[in] pickcollist    Column names to select (empty = keep all columns).
     * @param[in] pickncol       Expected number of picked columns (used when @p pickcollist is empty).
     *
     * @return A unique pointer to the picked sub-matrix, or @c nullptr if a
     *         requested name was not found.
     */
    std::unique_ptr<MneNamedMatrix> pick(const QStringList& pickrowlist,
                                         int picknrow,
                                         const QStringList& pickcollist,
                                         int pickncol) const;

public:
    int   nrow;             /**< Number of rows in @ref data. */
    int   ncol;             /**< Number of columns in @ref data. */
    QStringList rowlist;    /**< Name list for the rows (may be empty if unnamed). */
    QStringList collist;    /**< Name list for the columns (may be empty if unnamed). */
    Eigen::MatrixXf data;  /**< Dense data matrix of dimension @ref nrow x @ref ncol. */
};

} // NAMESPACE MNELIB

#endif // MNENAMEDMATRIX_H
