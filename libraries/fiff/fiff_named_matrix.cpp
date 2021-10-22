//=============================================================================================================
/**
 * @file     fiff_named_matrix.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the FiffNamedMatrix Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_named_matrix.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffNamedMatrix::FiffNamedMatrix()
: nrow(-1)
, ncol(-1)
{
}

//=============================================================================================================

FiffNamedMatrix::FiffNamedMatrix(fiff_int_t p_nrow, fiff_int_t p_ncol, const QStringList& p_row_names, const QStringList& p_col_names, const MatrixXd& p_data)
: nrow(p_nrow)
, ncol(p_ncol)
, row_names(p_row_names)
, col_names(p_col_names)
, data(p_data)
{
}

//=============================================================================================================

FiffNamedMatrix::FiffNamedMatrix(const FiffNamedMatrix& p_FiffNamedMatrix)
: QSharedData(p_FiffNamedMatrix)
, nrow(p_FiffNamedMatrix.nrow)
, ncol(p_FiffNamedMatrix.ncol)
, row_names(p_FiffNamedMatrix.row_names)
, col_names(p_FiffNamedMatrix.col_names)
, data(p_FiffNamedMatrix.data)
{
}

//=============================================================================================================

void FiffNamedMatrix::clear()
{
    nrow = -1;
    ncol = -1;
    row_names.clear();
    col_names.clear();
    data = MatrixXd();
}

//=============================================================================================================

void FiffNamedMatrix::transpose_named_matrix()
{
    QStringList col_names_old = this->col_names;
    this->col_names = this->row_names;
    this->row_names = col_names_old;

    this->data.transposeInPlace();

    this->nrow = this->data.rows();
    this->ncol = this->data.cols();
}
