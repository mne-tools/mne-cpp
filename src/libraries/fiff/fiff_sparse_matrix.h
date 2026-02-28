//=============================================================================================================
/**
 * @file     fiff_sparse_matrix.h
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
 * @brief    FiffSparseMatrix class declaration.
 *
 */

#ifndef FIFFSPARSEMATRIX_H
#define FIFFSPARSEMATRIX_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_types.h"
#include "fiff_tag.h"

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
#include <QDebug>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
/**
 * Implements a FIFF sparse matrix.
 *
 * @brief FIFF sparse matrix storage.
 */
class FIFFSHARED_EXPORT FiffSparseMatrix
{
public:
    typedef QSharedPointer<FiffSparseMatrix> SPtr;              /**< Shared pointer type for FiffSparseMatrix. */
    typedef QSharedPointer<const FiffSparseMatrix> ConstSPtr;   /**< Const shared pointer type for FiffSparseMatrix. */
    typedef std::unique_ptr<FiffSparseMatrix> UPtr;             /**< Unique pointer type for FiffSparseMatrix. */

    //=========================================================================================================
    /**
     * Constructs the FiffSparseMatrix
     */
    FiffSparseMatrix();

    //=========================================================================================================
    /**
     * Copies a FiffSparseMatrix (default — Eigen vectors handle deep copy).
     */
    FiffSparseMatrix(const FiffSparseMatrix& mat) = default;

    //=========================================================================================================
    /**
     * Default move constructor.
     */
    FiffSparseMatrix(FiffSparseMatrix&& mat) = default;

    //=========================================================================================================
    /**
     * Default copy-assignment operator.
     */
    FiffSparseMatrix& operator=(const FiffSparseMatrix&) = default;

    //=========================================================================================================
    /**
     * Default move-assignment operator.
     */
    FiffSparseMatrix& operator=(FiffSparseMatrix&&) = default;

    //=========================================================================================================
    /**
     * Destroys the FiffSparseMatrix (default — Eigen vectors clean up automatically).
     */
    ~FiffSparseMatrix() = default;

    //============================= fiff_sparse.c =============================

    /**
     * Interpret dimensions and nz from matrix data.
     *
     * @param[in] tag   The tag containing sparse matrix data.
     *
     * @return A vector with the matrix dimension info, or empty on error.
     */
    static std::vector<int> fiff_get_matrix_sparse_dims(FIFFLIB::FiffTag::SPtr& tag);

    /**
     * Conversion of tag data into the standard sparse representation.
     *
     * @param[in] tag   The tag containing sparse matrix data.
     *
     * @return A unique pointer to the newly constructed FiffSparseMatrix, or nullptr on error.
     */
    static FiffSparseMatrix::UPtr fiff_get_float_sparse_matrix(FIFFLIB::FiffTag::SPtr& tag);

    //============================= mne_sparse_matop.c =============================

    /**
     * Create a sparse RCS matrix from row-based data.
     *
     * @param[in] nrow      Number of rows.
     * @param[in] ncol      Number of columns.
     * @param[in] nnz       Number of non-zero elements on each row.
     * @param[in] colindex  Column indices of non-zero elements on each row.
     * @param[in] vals      Values of non-zero elements on each row.
     *
     * @return A unique pointer to the newly constructed FiffSparseMatrix, or nullptr on error.
     */
    static FiffSparseMatrix::UPtr create_sparse_rcs(int nrow,
                                                    int ncol,
                                                    int *nnz,
                                                    int **colindex,
                                                    float **vals);

    /**
     * Add the upper triangle to a lower-triangular sparse RCS matrix.
     *
     * @return A unique pointer to the newly constructed FiffSparseMatrix with both triangles.
     */
    FiffSparseMatrix::UPtr mne_add_upper_triangle_rcs();

    /**
     * Check whether this sparse matrix is empty (no non-zero elements).
     *
     * @return true if the matrix has no data.
     */
    inline bool is_empty() const;

public:
    FIFFLIB::fiff_int_t   coding;    /**< coding (storage) type of the sparse matrix. */
    FIFFLIB::fiff_int_t   m;         /**< m rows. */
    FIFFLIB::fiff_int_t   n;         /**< n columns. */
    FIFFLIB::fiff_int_t   nz;        /**< nz nonzeros. */
    Eigen::VectorXf       data;      /**< Non-zero values (nz elements). */
    Eigen::VectorXi       inds;      /**< Index array (nz elements). */
    Eigen::VectorXi       ptrs;      /**< Pointer array (m+1 for RCS, n+1 for CCS). */

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool FiffSparseMatrix::is_empty() const
{
    return nz <= 0 || data.size() == 0;
}

} // NAMESPACE FIFFLIB

#endif // FIFFSPARSEMATRIX_H
