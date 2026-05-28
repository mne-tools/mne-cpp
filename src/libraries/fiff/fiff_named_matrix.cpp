//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_named_matrix.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     October 2012
 * @brief    Implementation of @ref FiffNamedMatrix: FIFF-serializable matrix paired with row / column name lists.
 *
 * Used by SSP projectors, CTF compensators, forward gain matrices and
 * covariance / inverse operators.
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
