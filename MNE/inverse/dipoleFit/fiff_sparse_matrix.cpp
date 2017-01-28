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
