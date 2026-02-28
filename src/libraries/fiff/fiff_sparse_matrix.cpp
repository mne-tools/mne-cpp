//=============================================================================================================
/**
 * @file     fiff_sparse_matrix.cpp
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
 * @brief    Definition of the FiffSparseMatrix Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_sparse_matrix.h"
#include <fiff/fiff_file.h>
#include <fiff/fiff_types.h>

#include <vector>

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

std::vector<int> fiff_get_matrix_dims(FiffTag::SPtr& tag)
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
, m(0)
, n(0)
, nz(0)
{
}

//=============================================================================================================

std::vector<int> FiffSparseMatrix::fiff_get_matrix_sparse_dims(FiffTag::SPtr &tag)
{
    return fiff_get_matrix_dims(tag);
}

//=============================================================================================================

FiffSparseMatrix::UPtr FiffSparseMatrix::fiff_get_float_sparse_matrix(FiffTag::SPtr &tag)
{
    int   m,n,nz;
    int   coding,correct_size;

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

    coding = fiff_type_matrix_coding(tag->type);
    if (coding == FIFFTS_MC_CCS)
        correct_size = nz*(sizeof(fiff_float_t) + sizeof(fiff_int_t)) +
                (n+1+dims[0]+2)*(sizeof(fiff_int_t));
    else if (coding == FIFFTS_MC_RCS)
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
     * Parse tag data into Eigen vectors via Map (zero-copy wrap + assignment copy)
     */
    auto res = std::make_unique<FiffSparseMatrix>();
    res->m      = m;
    res->n      = n;
    res->nz     = nz;
    res->coding = coding;

    const float* src_data = reinterpret_cast<const float*>(tag->data());
    const int*   src_inds = reinterpret_cast<const int*>(src_data + nz);
    const int*   src_ptrs = src_inds + nz;
    int ptrs_count = (coding == FIFFTS_MC_CCS) ? (n + 1) : (m + 1);

    res->data = Eigen::Map<const Eigen::VectorXf>(src_data, nz);
    res->inds = Eigen::Map<const Eigen::VectorXi>(src_inds, nz);
    res->ptrs = Eigen::Map<const Eigen::VectorXi>(src_ptrs, ptrs_count);

    return res;
}

//=============================================================================================================

FiffSparseMatrix::UPtr FiffSparseMatrix::create_sparse_rcs(int nrow, int ncol, int *nnz, int **colindex, float **vals)
{
    int j,k,nz,ptr,ind;

    for (j = 0, nz = 0; j < nrow; j++)
        nz = nz + nnz[j];

    if (nz <= 0) {
        qWarning("[FiffSparseMatrix::create_sparse_rcs] No nonzero elements specified.");
        return nullptr;
    }

    auto sparse = std::make_unique<FiffSparseMatrix>();
    sparse->coding = FIFFTS_MC_RCS;
    sparse->m      = nrow;
    sparse->n      = ncol;
    sparse->nz     = nz;
    sparse->data   = Eigen::VectorXf::Zero(nz);
    sparse->inds   = Eigen::VectorXi::Zero(nz);
    sparse->ptrs   = Eigen::VectorXi::Zero(nrow + 1);

    for (j = 0, nz = 0; j < nrow; j++) {
        ptr = -1;
        for (k = 0; k < nnz[j]; k++) {
            if (ptr < 0)
                ptr = nz;
            ind = sparse->inds[nz] = colindex[j][k];
            if (ind < 0 || ind >= ncol) {
                qWarning("[FiffSparseMatrix::create_sparse_rcs] Column index out of range");
                return nullptr;
            }
            if (vals)
                sparse->data[nz] = vals[j][k];
            else
                sparse->data[nz] = 0.0;
            nz++;
        }
        sparse->ptrs[j] = ptr;
    }
    sparse->ptrs[nrow] = nz;
    for (j = nrow-1; j >= 0; j--) /* Take care of the empty rows */
        if (sparse->ptrs[j] < 0)
            sparse->ptrs[j] = sparse->ptrs[j+1];
    return sparse;
}

//=============================================================================================================

FiffSparseMatrix::UPtr FiffSparseMatrix::mne_add_upper_triangle_rcs()
/*
 * Fill in upper triangle with the lower triangle values
 */
{
    if (this->coding != FIFFTS_MC_RCS) {
        qWarning("[FiffSparseMatrix::mne_add_upper_triangle_rcs] input must be in RCS format");
        return nullptr;
    }
    if (this->m != this->n) {
        qWarning("[FiffSparseMatrix::mne_add_upper_triangle_rcs] input must be square");
        return nullptr;
    }

    // Build per-row data from existing lower triangle
    std::vector<int> nnz_vec(this->m);
    std::vector<std::vector<int>>   colindex(this->m);
    std::vector<std::vector<float>> vals(this->m);

    for (int i = 0; i < this->m; i++) {
        nnz_vec[i] = this->ptrs[i+1] - this->ptrs[i];
        if (nnz_vec[i] > 0) {
            colindex[i].resize(nnz_vec[i]);
            vals[i].resize(nnz_vec[i]);
            for (int j = this->ptrs[i], k = 0; j < this->ptrs[i+1]; j++, k++) {
                vals[i][k]     = this->data[j];
                colindex[i][k] = this->inds[j];
            }
        }
    }

    // Count additional upper-triangle entries per row
    std::vector<int> nadd(this->m, 0);
    for (int i = 0; i < this->m; i++)
        for (int j = this->ptrs[i]; j < this->ptrs[i+1]; j++)
            nadd[this->inds[j]]++;

    // Expand per-row storage and add upper-triangle entries
    for (int i = 0; i < this->m; i++) {
        colindex[i].resize(nnz_vec[i] + nadd[i]);
        vals[i].resize(nnz_vec[i] + nadd[i]);
    }
    for (int i = 0; i < this->m; i++)
        for (int j = this->ptrs[i]; j < this->ptrs[i+1]; j++) {
            int row = this->inds[j];
            colindex[row][nnz_vec[row]] = i;
            vals[row][nnz_vec[row]]     = this->data[j];
            nnz_vec[row]++;
        }

    // Build raw pointer arrays for create_sparse_rcs
    std::vector<int*>   ci_ptrs(this->m);
    std::vector<float*> val_ptrs(this->m);
    for (int i = 0; i < this->m; i++) {
        ci_ptrs[i]  = colindex[i].data();
        val_ptrs[i] = vals[i].data();
    }

    return create_sparse_rcs(this->m, this->n, nnz_vec.data(), ci_ptrs.data(), val_ptrs.data());
}
