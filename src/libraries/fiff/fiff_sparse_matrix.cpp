//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiff_sparse_matrix.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February 2026
 * @brief    Implementation of @ref FiffSparseMatrix: CCS / RCS storage matching the FIFF sparse-matrix tag layout, with Eigen conversion.
 *
 * Stores values, inner indices and outer pointers exactly as the FIFF
 * stream produced them so the matrix can be re-emitted without
 * re-encoding.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_sparse_matrix.h"
#include <fiff/fiff_file.h>
#include <fiff/fiff_types.h>

#include <vector>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;

//============================= fiff_type_spec.h =============================

/*
 * These return information about a fiff type.
 */

fiff_int_t fiff_type_base(fiff_int_t type)
{
    return type & FIFFTS_BASE_MASK;
}

fiff_int_t fiff_type_fundamental(fiff_int_t type)
{
    return type & FIFFTS_FS_MASK;
}

fiff_int_t fiff_type_matrix_coding(fiff_int_t type)
{
    return type & FIFFTS_MC_MASK;
}

//============================= fiff_matrix.c =============================

std::vector<int> fiff_get_matrix_dims(const FiffTag::UPtr& tag)
/*
      * Interpret dimensions from matrix data (dense and sparse)
      */
{
    int ndim;
    int *dims;
    unsigned int tsize = tag->size();
    /*
   * Initial checks
   */
    if (tag->data() == nullptr) {
        qCritical("fiff_get_matrix_dims: no data available!");
        return {};
    }
    if (fiff_type_fundamental(tag->type) != FIFFTS_FS_MATRIX) {
        qCritical("fiff_get_matrix_dims: tag does not contain a matrix!");
        return {};
    }
    if (tsize < sizeof(fiff_int_t)) {
        qCritical("fiff_get_matrix_dims: too small matrix data!");
        return {};
    }
    /*
   * Get the number of dimensions and check
   */
    ndim = *((fiff_int_t *)((fiff_byte_t *)(tag->data())+tag->size()-sizeof(fiff_int_t)));
    if (ndim <= 0 || ndim > FIFFC_MATRIX_MAX_DIM) {
        qCritical("fiff_get_matrix_dims: unreasonable # of dimensions!");
        return {};
    }
    if (fiff_type_matrix_coding(tag->type) == FIFFTS_MC_DENSE) {
        if (tsize < (ndim+1)*sizeof(fiff_int_t)) {
            qCritical("fiff_get_matrix_dims: too small matrix data!");
            return {};
        }
        std::vector<int> res(ndim + 1);
        res[0] = ndim;
        dims = ((fiff_int_t *)((fiff_byte_t *)(tag->data())+tag->size())) - ndim - 1;
        for (int k = 0; k < ndim; k++)
            res[k+1] = dims[k];
        return res;
    }
    else if (fiff_type_matrix_coding(tag->type) == FIFFTS_MC_CCS ||
             fiff_type_matrix_coding(tag->type) == FIFFTS_MC_RCS) {
        if (tsize < (ndim+2)*sizeof(fiff_int_t)) {
            qCritical("fiff_get_matrix_sparse_dims: too small matrix data!");
            return {};
        }
        std::vector<int> res(ndim + 2);
        res[0] = ndim;
        dims = ((fiff_int_t *)((fiff_byte_t *)(tag->data())+tag->size())) - ndim - 1;
        for (int k = 0; k < ndim; k++)
            res[k+1] = dims[k];
        res[ndim+1] = dims[-1];
        return res;
    }
    else {
        qCritical("fiff_get_matrix_dims: unknown matrix coding.");
        return {};
    }
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffSparseMatrix::FiffSparseMatrix()
: coding(0)
{
}

//=============================================================================================================

FiffSparseMatrix::FiffSparseMatrix(Eigen::SparseMatrix<float>&& mat,
                                   FIFFLIB::fiff_int_t coding)
: coding(coding)
, m_eigen(std::move(mat))
{
}

//=============================================================================================================

std::vector<int> FiffSparseMatrix::fiff_get_matrix_sparse_dims(const FiffTag::UPtr& tag)
{
    return fiff_get_matrix_dims(tag);
}

//=============================================================================================================

FiffSparseMatrix::UPtr FiffSparseMatrix::fiff_get_float_sparse_matrix(const FiffTag::UPtr& tag)
{
    int   m,n,nz;
    int   cod,correct_size;

    if ( fiff_type_fundamental(tag->type)   != FIFFT_MATRIX ||
         fiff_type_base(tag->type)          != FIFFT_FLOAT ||
         (fiff_type_matrix_coding(tag->type) != FIFFTS_MC_CCS &&
          fiff_type_matrix_coding(tag->type) != FIFFTS_MC_RCS) ) {
        qWarning("[FiffSparseMatrix::fiff_get_float_sparse_matrix] wrong data type!");
        return nullptr;
    }

    auto dims = fiff_get_matrix_sparse_dims(tag);
    if (dims.empty())
        return nullptr;

    if (dims[0] != 2) {
        qWarning("[FiffSparseMatrix::fiff_get_float_sparse_matrix] wrong # of dimensions!");
        return nullptr;
    }

    m   = dims[1];
    n   = dims[2];
    nz  = dims[3];

    cod = fiff_type_matrix_coding(tag->type);
    if (cod == FIFFTS_MC_CCS)
        correct_size = nz*(sizeof(fiff_float_t) + sizeof(fiff_int_t)) +
                (n+1+dims[0]+2)*(sizeof(fiff_int_t));
    else if (cod == FIFFTS_MC_RCS)
        correct_size = nz*(sizeof(fiff_float_t) + sizeof(fiff_int_t)) +
                (m+1+dims[0]+2)*(sizeof(fiff_int_t));
    else {
        qWarning("[FiffSparseMatrix::fiff_get_float_sparse_matrix] Incomprehensible sparse matrix coding");
        return nullptr;
    }
    if (tag->size() != correct_size) {
        qWarning("[FiffSparseMatrix::fiff_get_float_sparse_matrix] wrong data size!");
        return nullptr;
    }
    /*
     * Parse tag data into triplets and build Eigen sparse matrix directly
     */
    const float* src_data = reinterpret_cast<const float*>(tag->data());
    const int*   src_inds = reinterpret_cast<const int*>(src_data + nz);
    const int*   src_ptrs = src_inds + nz;

    using T = Eigen::Triplet<float>;
    std::vector<T> triplets;
    triplets.reserve(nz);

    if (cod == FIFFTS_MC_RCS) {
        for (int row = 0; row < m; ++row) {
            for (int j = src_ptrs[row]; j < src_ptrs[row + 1]; ++j) {
                triplets.push_back(T(row, src_inds[j], src_data[j]));
            }
        }
    } else { // CCS
        for (int col = 0; col < n; ++col) {
            for (int j = src_ptrs[col]; j < src_ptrs[col + 1]; ++j) {
                triplets.push_back(T(src_inds[j], col, src_data[j]));
            }
        }
    }

    Eigen::SparseMatrix<float> eigenMat(m, n);
    eigenMat.setFromTriplets(triplets.begin(), triplets.end());
    eigenMat.makeCompressed();

    auto res = std::make_unique<FiffSparseMatrix>(std::move(eigenMat), cod);
    return res;
}

//=============================================================================================================

FiffSparseMatrix::UPtr FiffSparseMatrix::create_sparse_rcs(int nrow, int ncol, int *nnz, int **colindex, float **vals)
{
    int j,k,totalNz;

    for (j = 0, totalNz = 0; j < nrow; j++)
        totalNz = totalNz + nnz[j];

    if (totalNz <= 0) {
        qWarning("[FiffSparseMatrix::create_sparse_rcs] No nonzero elements specified.");
        return nullptr;
    }

    using T = Eigen::Triplet<float>;
    std::vector<T> triplets;
    triplets.reserve(totalNz);

    for (j = 0; j < nrow; j++) {
        for (k = 0; k < nnz[j]; k++) {
            int col = colindex[j][k];
            if (col < 0 || col >= ncol) {
                qWarning("[FiffSparseMatrix::create_sparse_rcs] Column index out of range");
                return nullptr;
            }
            float val = vals ? vals[j][k] : 0.0f;
            triplets.push_back(T(j, col, val));
        }
    }

    Eigen::SparseMatrix<float> eigenMat(nrow, ncol);
    eigenMat.setFromTriplets(triplets.begin(), triplets.end());
    eigenMat.makeCompressed();

    return std::make_unique<FiffSparseMatrix>(std::move(eigenMat), FIFFTS_MC_RCS);
}

//=============================================================================================================

FiffSparseMatrix::UPtr FiffSparseMatrix::mne_add_upper_triangle_rcs()
/*
 * Fill in upper triangle with the lower triangle values
 */
{
    int nRows = rows();
    int nCols = cols();

    if (nRows != nCols) {
        qWarning("[FiffSparseMatrix::mne_add_upper_triangle_rcs] input must be square");
        return nullptr;
    }

    // Build full (lower + upper) by adding transpose
    Eigen::SparseMatrix<float> full = m_eigen + Eigen::SparseMatrix<float>(m_eigen.transpose());

    // The diagonal was counted twice — fix by subtracting the diagonal once
    for (int k = 0; k < full.outerSize(); ++k) {
        for (Eigen::SparseMatrix<float>::InnerIterator it(full, k); it; ++it) {
            if (it.row() == it.col()) {
                // Original diagonal value is in m_eigen; full has 2x, so set back to 1x
                it.valueRef() = 0.5f * it.value();
            }
        }
    }

    full.makeCompressed();
    return std::make_unique<FiffSparseMatrix>(std::move(full), FIFFTS_MC_RCS);
}

//=============================================================================================================

FiffSparseMatrix FiffSparseMatrix::fromEigenSparse(const Eigen::SparseMatrix<double>& mat)
{
    FiffSparseMatrix result;
    if (mat.nonZeros() == 0)
        return result;

    result.coding = FIFFTS_MC_RCS;
    result.m_eigen = mat.cast<float>();
    result.m_eigen.makeCompressed();
    return result;
}

//=============================================================================================================

FiffSparseMatrix FiffSparseMatrix::fromEigenSparse(const Eigen::SparseMatrix<float>& mat)
{
    FiffSparseMatrix result;
    if (mat.nonZeros() == 0)
        return result;

    result.coding = FIFFTS_MC_RCS;
    result.m_eigen = mat;
    result.m_eigen.makeCompressed();
    return result;
}

//=============================================================================================================

FiffSparseMatrix::UPtr FiffSparseMatrix::pickLowerTriangleRcs() const
{
    int nRows = rows();
    int nCols = cols();

    if (nRows != nCols) {
        qWarning("[FiffSparseMatrix::pickLowerTriangleRcs] input must be square");
        return nullptr;
    }

    using T = Eigen::Triplet<float>;
    std::vector<T> triplets;
    triplets.reserve(m_eigen.nonZeros());

    for (int k = 0; k < m_eigen.outerSize(); ++k) {
        for (Eigen::SparseMatrix<float>::InnerIterator it(m_eigen, k); it; ++it) {
            if (it.row() >= it.col()) {  // lower triangle including diagonal
                triplets.push_back(T(it.row(), it.col(), it.value()));
            }
        }
    }

    Eigen::SparseMatrix<float> lower(nRows, nCols);
    lower.setFromTriplets(triplets.begin(), triplets.end());
    lower.makeCompressed();

    return std::make_unique<FiffSparseMatrix>(std::move(lower), FIFFTS_MC_RCS);
}
