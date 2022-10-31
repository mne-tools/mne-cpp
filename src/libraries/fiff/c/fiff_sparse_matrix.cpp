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

#define MALLOC_18(x,t) (t *)malloc((x)*sizeof(t))

#define REALLOC_18(x,y,t) (t *)((x == NULL) ? malloc((y)*sizeof(t)) : realloc((x),(y)*sizeof(t)))

#define FREE_18(x) if ((char *)(x) != NULL) free((char *)(x))

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
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

int *fiff_get_matrix_dims(FiffTag::SPtr& tag)
/*
      * Interpret dimensions from matrix data (dense and sparse)
      */
{
    int ndim;
    int *dims;
    int *res,k;
    unsigned int tsize = tag->size();
    /*
   * Initial checks
   */
    if (tag->data() == NULL) {
        qCritical("fiff_get_matrix_dims: no data available!");
        return NULL;
    }
    if (fiff_type_fundamental(tag->type) != FIFFTS_FS_MATRIX) {
        qCritical("fiff_get_matrix_dims: tag does not contain a matrix!");
        return NULL;
    }
    if (tsize < sizeof(fiff_int_t)) {
        qCritical("fiff_get_matrix_dims: too small matrix data!");
        return NULL;
    }
    /*
   * Get the number of dimensions and check
   */
    ndim = *((fiff_int_t *)((fiff_byte_t *)(tag->data())+tag->size()-sizeof(fiff_int_t)));
    if (ndim <= 0 || ndim > FIFFC_MATRIX_MAX_DIM) {
        qCritical("fiff_get_matrix_dims: unreasonable # of dimensions!");
        return NULL;
    }
    if (fiff_type_matrix_coding(tag->type) == FIFFTS_MC_DENSE) {
        if (tsize < (ndim+1)*sizeof(fiff_int_t)) {
            qCritical("fiff_get_matrix_dims: too small matrix data!");
            return NULL;
        }
        res = MALLOC_18(ndim+1,int);
        res[0] = ndim;
        dims = ((fiff_int_t *)((fiff_byte_t *)(tag->data())+tag->size())) - ndim - 1;
        for (k = 0; k < ndim; k++)
            res[k+1] = dims[k];
    }
    else if (fiff_type_matrix_coding(tag->type) == FIFFTS_MC_CCS ||
             fiff_type_matrix_coding(tag->type) == FIFFTS_MC_RCS) {
        if (tsize < (ndim+2)*sizeof(fiff_int_t)) {
            qCritical("fiff_get_matrix_sparse_dims: too small matrix data!");
            return NULL; }

        res = MALLOC_18(ndim+2,int);
        res[0] = ndim;
        dims = ((fiff_int_t *)((fiff_byte_t *)(tag->data())+tag->size())) - ndim - 1;
        for (k = 0; k < ndim; k++)
            res[k+1] = dims[k];
        res[ndim+1] = dims[-1];
    }
    else {
        qCritical("fiff_get_matrix_dims: unknown matrix coding.");
        return NULL;
    }
    return res;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffSparseMatrix::FiffSparseMatrix()
: coding(0)
, m(0)
, n(0)
, nz(0)
, data(NULL)
, inds(NULL)
, ptrs(NULL)
{
}

//=============================================================================================================

FiffSparseMatrix::FiffSparseMatrix(const FiffSparseMatrix &mat)
{
    int             size;

    this->coding = mat.coding;
    this->m      = mat.m;
    this->n      = mat.n;
    this->nz     = mat.nz;

    if (mat.coding == FIFFTS_MC_CCS) {
        size = mat.nz*(sizeof(FIFFLIB::fiff_float_t) + sizeof(FIFFLIB::fiff_int_t)) +
                (mat.n+1)*(sizeof(FIFFLIB::fiff_int_t));
    }
    if (mat.coding == FIFFTS_MC_RCS) {
        size = mat.nz*(sizeof(FIFFLIB::fiff_float_t) + sizeof(FIFFLIB::fiff_int_t)) +
                (mat.m+1)*(sizeof(FIFFLIB::fiff_int_t));
    }
    else {
        printf("Illegal sparse matrix storage type: %d",mat.coding);
        return;
    }
    this->data   = (float *)malloc(size);
    this->inds   = (int *)(this->data+this->nz);
    this->ptrs   = this->inds+this->nz;
    memcpy(data,mat.data,size);
}

//=============================================================================================================

FiffSparseMatrix::~FiffSparseMatrix()
{
    if(data)
        FREE_18(data);
}

//=============================================================================================================

fiff_int_t *FiffSparseMatrix::fiff_get_matrix_sparse_dims(FiffTag::SPtr &tag)
{
    return fiff_get_matrix_dims(tag);
}

//=============================================================================================================

FiffSparseMatrix *FiffSparseMatrix::fiff_get_float_sparse_matrix(FiffTag::SPtr &tag)
{
    int *dims;
    FIFFLIB::FiffSparseMatrix* res = NULL;
    int   m,n,nz;
    int   coding,correct_size;

    if ( fiff_type_fundamental(tag->type)   != FIFFT_MATRIX ||
         fiff_type_base(tag->type)          != FIFFT_FLOAT ||
         (fiff_type_matrix_coding(tag->type) != FIFFTS_MC_CCS &&
          fiff_type_matrix_coding(tag->type) != FIFFTS_MC_RCS) ) {
        printf("fiff_get_float_ccs_matrix: wrong data type!");
        return NULL;
    }

    if ((dims = fiff_get_matrix_sparse_dims(tag)) == NULL)
        return NULL;

    if (dims[0] != 2) {
        printf("fiff_get_float_sparse_matrix: wrong # of dimensions!");
        return NULL;
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
        printf("fiff_get_float_sparse_matrix: Incomprehensible sparse matrix coding");
        return NULL;
    }
    if (tag->size() != correct_size) {
        printf("fiff_get_float_sparse_matrix: wrong data size!");
        FREE_18(dims);
        return NULL;
    }
    /*
        * Set up structure
        */
    res = new FIFFLIB::FiffSparseMatrix;
    res->m      = m;
    res->n      = n;
    res->nz     = nz;
    res->data   = MALLOC_18(correct_size,float);
    memcpy (res->data,(float*)tag->data(),correct_size);
    res->coding = coding;
    res->inds   = (int *)(res->data + res->nz);
    res->ptrs   = res->inds + res->nz;

    FREE_18(dims);

    return res;
}

//=============================================================================================================

FiffSparseMatrix *FiffSparseMatrix::create_sparse_rcs(int nrow, int ncol, int *nnz, int **colindex, float **vals) 	     /* The nonzero elements on each row
                                                                  * If null, the matrix will be all zeroes */
{
    FIFFLIB::FiffSparseMatrix* sparse = NULL;
    int j,k,nz,ptr,size,ind;
    int stor_type = FIFFTS_MC_RCS;

    for (j = 0, nz = 0; j < nrow; j++)
        nz = nz + nnz[j];

    if (nz <= 0) {
        printf("No nonzero elements specified.");
        return NULL;
    }
    if (stor_type == FIFFTS_MC_RCS) {
        size = nz*(sizeof(fiff_float_t) + sizeof(fiff_int_t)) +
                (nrow+1)*(sizeof(fiff_int_t));
    }
    else {
        printf("Illegal sparse matrix storage type: %d",stor_type);
        return NULL;
    }
    sparse = new FIFFLIB::FiffSparseMatrix;
    sparse->coding = stor_type;
    sparse->m      = nrow;
    sparse->n      = ncol;
    sparse->nz     = nz;
    sparse->data   = (float *)malloc(size);
    sparse->inds   = (int *)(sparse->data+nz);
    sparse->ptrs   = sparse->inds+nz;

    for (j = 0, nz = 0; j < nrow; j++) {
        ptr = -1;
        for (k = 0; k < nnz[j]; k++) {
            if (ptr < 0)
                ptr = nz;
            ind = sparse->inds[nz] = colindex[j][k];
            if (ind < 0 || ind >= ncol) {
                printf("Column index out of range in mne_create_sparse_rcs");
                goto bad;
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

bad : {
        if(sparse)
            delete sparse;
        return NULL;
    }
}

//=============================================================================================================

FiffSparseMatrix *FiffSparseMatrix::mne_add_upper_triangle_rcs()
/*
     * Fill in upper triangle with the lower triangle values
     */
{
    int *nnz       = NULL;
    int **colindex = NULL;
    float **vals   = NULL;
    FIFFLIB::FiffSparseMatrix* res = NULL;
    int i,j,k,row;
    int *nadd = NULL;

    if (this->coding != FIFFTS_MC_RCS) {
        printf("The input matrix to mne_add_upper_triangle_rcs must be in RCS format");
        goto out;
    }
    if (this->m != this->n) {
        printf("The input matrix to mne_add_upper_triangle_rcs must be square");
        goto out;
    }
    nnz      = MALLOC_18(this->m,int);
    colindex = MALLOC_18(this->m,int *);
    vals     = MALLOC_18(this->m,float *);
    for (i = 0; i < this->m; i++) {
        nnz[i]      = this->ptrs[i+1] - this->ptrs[i];
        if (nnz[i] > 0) {
            colindex[i] = MALLOC_18(nnz[i],int);
            vals[i]   = MALLOC_18(nnz[i],float);
            for (j = this->ptrs[i], k = 0; j < this->ptrs[i+1]; j++, k++) {
                vals[i][k] = this->data[j];
                colindex[i][k] = this->inds[j];
            }
        }
        else {
            colindex[i] = NULL;
            vals[i] = NULL;
        }
    }
    /*
        * Add the elements
        */
    nadd = MALLOC_18(this->m,int);
    for (i = 0; i < this->m; i++)
        nadd[i] = 0;
    for (i = 0; i < this->m; i++)
        for (j = this->ptrs[i]; j < this->ptrs[i+1]; j++)
            nadd[this->inds[j]]++;
    for (i = 0; i < this->m; i++) {
        colindex[i] = REALLOC_18(colindex[i],nnz[i]+nadd[i],int);
        vals[i]     = REALLOC_18(vals[i],nnz[i]+nadd[i],float);
    }
    for (i = 0; i < this->m; i++)
        for (j = this->ptrs[i]; j < this->ptrs[i+1]; j++) {
            row = this->inds[j];
            colindex[row][nnz[row]] = i;
            vals[row][nnz[row]]     = this->data[j];
            nnz[row]++;
        }
    res = create_sparse_rcs(this->m,this->n,nnz,colindex,vals);

out : {
        for (i = 0; i < this->m; i++) {
            FREE_18(colindex[i]);
            FREE_18(vals[i]);
        }
        FREE_18(nnz);
        FREE_18(vals);
        FREE_18(colindex);
        FREE_18(nadd);
        return res;
    }
}
