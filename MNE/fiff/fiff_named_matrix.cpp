//=============================================================================================================
/**
* @file     fiff_named_matrix.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains the implementation of the FiffNamedMatrix Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_named_matrix.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffNamedMatrix::FiffNamedMatrix()
: nrow(-1)
, ncol(-1)
, data(NULL)
{
}


//*************************************************************************************************************

FiffNamedMatrix::FiffNamedMatrix(fiff_int_t p_nrow, fiff_int_t p_ncol, QStringList& p_row_names, QStringList& p_col_names, MatrixXd* p_data)
: nrow(p_nrow)
, ncol(p_ncol)
, row_names(p_row_names)
, col_names(p_col_names)
, data(p_data ? new MatrixXd(*p_data) : NULL)
{
}


//*************************************************************************************************************

FiffNamedMatrix::FiffNamedMatrix(const FiffNamedMatrix* p_pFiffNamedMatrix)
: nrow(p_pFiffNamedMatrix->nrow)
, ncol(p_pFiffNamedMatrix->ncol)
, row_names(p_pFiffNamedMatrix->row_names)
, col_names(p_pFiffNamedMatrix->col_names)
, data(p_pFiffNamedMatrix->data ? new MatrixXd(*p_pFiffNamedMatrix->data) : NULL)
{
}


//*************************************************************************************************************

FiffNamedMatrix::~FiffNamedMatrix()
{
    if(data)
        delete data;
}


//*************************************************************************************************************

void FiffNamedMatrix::transpose_named_matrix()
{
    QStringList col_names_old = this->col_names;
    this->col_names = this->row_names;
    this->row_names = col_names_old;

    this->data->transposeInPlace();

    this->nrow = this->data->rows();
    this->ncol = this->data->cols();
}
