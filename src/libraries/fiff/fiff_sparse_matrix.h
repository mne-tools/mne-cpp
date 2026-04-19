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
#include <Eigen/SparseCore>

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
 * FIFF sparse matrix — wraps Eigen::SparseMatrix<float>.
 *
 * Internally stores data in a single Eigen::SparseMatrix<float>.
 * The @c coding member records whether the FIFF source was CCS or RCS
 * (used only during serialization / deserialization).
 *
 * @brief FIFF sparse matrix storage backed by Eigen.
 */
class FIFFSHARED_EXPORT FiffSparseMatrix
{
public:
    using SPtr = QSharedPointer<FiffSparseMatrix>;            /**< Shared pointer type for FiffSparseMatrix. */
    using ConstSPtr = QSharedPointer<const FiffSparseMatrix>; /**< Const shared pointer type for FiffSparseMatrix. */
    using UPtr = std::unique_ptr<FiffSparseMatrix>;           /**< Unique pointer type for FiffSparseMatrix. */
    using ConstUPtr = std::unique_ptr<const FiffSparseMatrix>;  /**< Const unique pointer type for FiffSparseMatrix. */

    //=========================================================================================================
    /**
     * Constructs an empty FiffSparseMatrix.
     */
    FiffSparseMatrix();

    //=========================================================================================================
    /**
     * Constructs a FiffSparseMatrix wrapping an existing Eigen sparse matrix.
     *
     * @param[in] mat     Eigen sparse matrix to wrap (moved in).
     * @param[in] coding  FIFF storage coding (FIFFTS_MC_RCS or FIFFTS_MC_CCS).
     */
    explicit FiffSparseMatrix(Eigen::SparseMatrix<float>&& mat,
                              FIFFLIB::fiff_int_t coding = FIFFTS_MC_RCS);

    //=========================================================================================================
    /**
     * Default copy constructor.
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
     * Default destructor.
     */
    ~FiffSparseMatrix() = default;

    //============================= Eigen access =============================

    /**
     * Mutable access to the underlying Eigen sparse matrix.
     * @return Reference to the internal Eigen::SparseMatrix<float>.
     */
    inline Eigen::SparseMatrix<float>& eigen() { return m_eigen; }

    /**
     * Const access to the underlying Eigen sparse matrix.
     * @return Const reference to the internal Eigen::SparseMatrix<float>.
     */
    inline const Eigen::SparseMatrix<float>& eigen() const { return m_eigen; }

    /**
     * Implicit conversion to const Eigen::SparseMatrix<float>&.
     */
    inline operator const Eigen::SparseMatrix<float>&() const { return m_eigen; }

    /**
     * Number of rows.
     */
    inline int rows() const { return static_cast<int>(m_eigen.rows()); }

    /**
     * Number of columns.
     */
    inline int cols() const { return static_cast<int>(m_eigen.cols()); }

    /**
     * Number of stored non-zero elements.
     */
    inline int nonZeros() const { return static_cast<int>(m_eigen.nonZeros()); }

    //============================= fiff_sparse.c =============================

    /**
     * Interpret dimensions and nz from matrix data.
     *
     * @param[in] tag   The tag containing sparse matrix data.
     *
     * @return A vector with the matrix dimension info, or empty on error.
     */
    static std::vector<int> fiff_get_matrix_sparse_dims(const FIFFLIB::FiffTag::UPtr& tag);

    /**
     * Conversion of tag data into the standard sparse representation.
     *
     * @param[in] tag   The tag containing sparse matrix data.
     *
     * @return A unique pointer to the newly constructed FiffSparseMatrix, or nullptr on error.
     */
    static FiffSparseMatrix::UPtr fiff_get_float_sparse_matrix(const FIFFLIB::FiffTag::UPtr& tag);

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
     * Extract only the lower triangle (including diagonal) from a square RCS matrix.
     *
     * @return A unique pointer to the newly constructed FiffSparseMatrix with lower-triangle elements.
     */
    FiffSparseMatrix::UPtr pickLowerTriangleRcs() const;

    /**
     * Check whether this sparse matrix is empty (no non-zero elements).
     *
     * @return true if the matrix has no data.
     */
    inline bool is_empty() const;

    /**
     * Convert to Eigen::SparseMatrix<double> (cast from float).
     *
     * @return A double-precision copy of the internal sparse matrix.
     */
    inline Eigen::SparseMatrix<double> toEigenSparse() const;

    /**
     * Create a FiffSparseMatrix from an Eigen::SparseMatrix<double>.
     * The data is cast to float for internal storage.
     *
     * @param[in] mat  The Eigen sparse matrix to convert.
     *
     * @return The equivalent FiffSparseMatrix.
     */
    static FiffSparseMatrix fromEigenSparse(const Eigen::SparseMatrix<double>& mat);

    /**
     * Create a FiffSparseMatrix from an Eigen::SparseMatrix<float>.
     *
     * @param[in] mat  The Eigen sparse matrix to wrap.
     *
     * @return The equivalent FiffSparseMatrix.
     */
    static FiffSparseMatrix fromEigenSparse(const Eigen::SparseMatrix<float>& mat);

public:
    FIFFLIB::fiff_int_t   coding;    /**< FIFF coding type (FIFFTS_MC_RCS or FIFFTS_MC_CCS). Used for serialization only. */

private:
    Eigen::SparseMatrix<float> m_eigen;  /**< The sparse matrix data. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool FiffSparseMatrix::is_empty() const
{
    return m_eigen.nonZeros() <= 0;
}

inline Eigen::SparseMatrix<double> FiffSparseMatrix::toEigenSparse() const
{
    return m_eigen.cast<double>();
}

} // NAMESPACE FIFFLIB

#endif // FIFFSPARSEMATRIX_H
