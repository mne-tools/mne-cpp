//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file fiff_sparse_matrix.h
 * @since February 2026
 * @brief FIFF sparse matrix: column / row-compressed sparse storage backed by Eigen::SparseMatrix.
 *
 * The FIFF matrix tag format supports three storage modes encoded in the
 * type word (see @ref fiff_tag.h): dense, column-compressed sparse
 * (@c MATRIX_CODING_CCS) and row-compressed sparse (@c MATRIX_CODING_RCS).
 * @ref FiffSparseMatrix is the in-memory representation of the two sparse
 * forms. It owns the value array, the index array and the pointer array
 * in the layout the FIFF stream produced, and exposes a conversion to
 * @c Eigen::SparseMatrix so downstream linear-algebra code can operate
 * on it directly. This is what backs large sparse blocks such as the
 * source-space adjacency, the SourceSpace patch matrices and the
 * volume-source-space neighbourhood matrices.
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
 * @brief Sparse FIFF matrix: CCS or RCS storage with the value / index / pointer triple as written by FiffStream::write_float_sparse_*.
 *
 * Holds the three arrays the FIFF sparse-matrix tag stores on disk
 * (values, inner indices, outer pointers) and the (nrow, ncol, nnz)
 * shape, so it can be streamed in and out without conversion. Convert to
 * an @c Eigen::SparseMatrix for arithmetic and back for serialization.
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
