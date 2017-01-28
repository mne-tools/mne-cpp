//=============================================================================================================
/**
* @file     fiff_sparse_matrix.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the FiffSparseMatrix Class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_sparse_matrix.h"
#include <fiff/fiff_file.h>
#include <fiff/fiff_types.h>


#define FREE_18(x) if ((char *)(x) != NULL) free((char *)(x))


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace INVERSELIB;


//*************************************************************************************************************
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


//*************************************************************************************************************

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


//*************************************************************************************************************

FiffSparseMatrix::~FiffSparseMatrix()
{
    if(data)
        FREE_18(data);
}


//*************************************************************************************************************

fiff_int_t *FiffSparseMatrix::fiff_get_matrix_sparse_dims(FiffTag::SPtr &tag)
{
    return fiff_get_matrix_dims(tag);
}


//*************************************************************************************************************

FiffSparseMatrix *FiffSparseMatrix::fiff_get_float_sparse_matrix(FiffTag::SPtr &tag)
{
    int *dims;
    INVERSELIB::FiffSparseMatrix* res = NULL;
    int   m,n,nz;
    int   coding,correct_size;

    if ( fiff_type_fundamental(tag->getType())   != FIFFT_MATRIX ||
         fiff_type_base(tag->getType())          != FIFFT_FLOAT ||
         (fiff_type_matrix_coding(tag->getType()) != FIFFTS_MC_CCS &&
          fiff_type_matrix_coding(tag->getType()) != FIFFTS_MC_RCS) ) {
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

    coding = fiff_type_matrix_coding(tag->getType());
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
        FREE(dims);
        return NULL;
    }
    /*
        * Set up structure
        */
    res = new INVERSELIB::FiffSparseMatrix;
    res->m      = m;
    res->n      = n;
    res->nz     = nz;
    qDebug() << "ToDo: Check if data are correctly set!";
    res->data   = tag->toFloat();
    res->coding = coding;
    res->inds   = (int *)(res->data + res->nz);
    res->ptrs   = res->inds + res->nz;

    FREE(dims);

    return res;
}


//*************************************************************************************************************

FiffSparseMatrix *FiffSparseMatrix::mne_add_upper_triangle_rcs()
/*
    * Fill in upper triangle with the lower triangle values
    */
{
    int *nnz       = NULL;
    int **colindex = NULL;
    float **vals   = NULL;
    INVERSELIB::FiffSparseMatrix* res = NULL;
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
    nnz      = MALLOC(this->m,int);
    colindex = MALLOC(this->m,int *);
    vals     = MALLOC(this->m,float *);
    for (i = 0; i < this->m; i++) {
        nnz[i]      = this->ptrs[i+1] - this->ptrs[i];
        if (nnz[i] > 0) {
            colindex[i] = MALLOC(nnz[i],int);
            vals[i]   = MALLOC(nnz[i],float);
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
    nadd = MALLOC(this->m,int);
    for (i = 0; i < this->m; i++)
        nadd[i] = 0;
    for (i = 0; i < this->m; i++)
        for (j = this->ptrs[i]; j < this->ptrs[i+1]; j++)
            nadd[this->inds[j]]++;
    for (i = 0; i < this->m; i++) {
        colindex[i] = REALLOC(colindex[i],nnz[i]+nadd[i],int);
        vals[i]     = REALLOC(vals[i],nnz[i]+nadd[i],float);
    }
    for (i = 0; i < this->m; i++)
        for (j = this->ptrs[i]; j < this->ptrs[i+1]; j++) {
            row = this->inds[j];
            colindex[row][nnz[row]] = i;
            vals[row][nnz[row]]     = this->data[j];
            nnz[row]++;
        }
    res = mne_create_sparse_rcs(this->m,this->n,nnz,colindex,vals);

out : {
        for (i = 0; i < this->m; i++) {
            FREE(colindex[i]);
            FREE(vals[i]);
        }
        FREE(nnz);
        FREE(vals);
        FREE(colindex);
        FREE(nadd);
        return res;
    }
}
