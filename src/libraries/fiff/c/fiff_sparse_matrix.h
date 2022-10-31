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

#include "../fiff_global.h"
#include "../fiff_types.h"
#include "../fiff_tag.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

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
 * Implements MNE Mne Data (Replaces *mneMneData,mneMneDataRec; struct of MNE-C mne_types.h).
 *
 * @brief Data associated with MNE computations for each mneMeasDataSet
 */
class FIFFSHARED_EXPORT FiffSparseMatrix
{
public:
    typedef QSharedPointer<FiffSparseMatrix> SPtr;              /**< Shared pointer type for FiffSparseMatrix. */
    typedef QSharedPointer<const FiffSparseMatrix> ConstSPtr;   /**< Const shared pointer type for FiffSparseMatrix. */

    //=========================================================================================================
    /**
     * Constructs the FiffSparseMatrix
     */
    FiffSparseMatrix();

    //=========================================================================================================
    /**
     * Copies a FiffSparseMatrix
     * Refactored: mne_dup_sparse_matrix (mne_sparse_matop.c)
     *
     * @param[in] mat     The Sparse Matrix which should be copied.
     */
    FiffSparseMatrix(const FiffSparseMatrix& mat);

    //=========================================================================================================
    /**
     * Destroys the FiffSparseMatrix description
     * Refactored: mne_free_sparse (mne_sparse_matop.c)
     */
    ~FiffSparseMatrix();

    //============================= fiff_sparse.c =============================
    /*
     * Interpret dimensions and nz from matrix data
     */
    static FIFFLIB::fiff_int_t *fiff_get_matrix_sparse_dims(FIFFLIB::FiffTag::SPtr& tag);

    /*
     * Conversion into the standard representation
     */
    static FIFFLIB::FiffSparseMatrix* fiff_get_float_sparse_matrix(FIFFLIB::FiffTag::SPtr& tag);

    //============================= mne_sparse_matop.c =============================

    //Refactored: mne_create_sparse_rcs
    static FIFFLIB::FiffSparseMatrix* create_sparse_rcs( int nrow,       /* Number of rows */
                                                            int ncol,       /* Number of columns */
                                                            int *nnz,       /* Number of non-zero elements on each row */
                                                            int **colindex, /* Column indices of non-zero elements on each row */
                                                            float **vals);

    FIFFLIB::FiffSparseMatrix* mne_add_upper_triangle_rcs();

public:
    FIFFLIB::fiff_int_t   coding;    /**< coding (storage) type of the sparse matrix. */
    FIFFLIB::fiff_int_t   m;         /**< m rows. */
    FIFFLIB::fiff_int_t   n;         /**< n columns. */
    FIFFLIB::fiff_int_t   nz;        /**< nz nonzeros. */
    FIFFLIB::fiff_float_t *data;     /**< owns the data. */
    FIFFLIB::fiff_int_t   *inds;     /**< index list, points into data, no dealloc!. */
    FIFFLIB::fiff_int_t   *ptrs;     /**< pointer list, points into data, no dealloc!. */

// ### OLD STRUCT ###
///** Structure for sparse matrices */
//typedef struct _fiff_sparse_matrix {
//    fiff_int_t   coding;    /**< coding (storage) type of the sparse matrix. */
//    fiff_int_t   m;         /**< m rows. */
//    fiff_int_t   n;         /**< n columns. */
//    fiff_int_t   nz;        /**< nz nonzeros. */
//    fiff_float_t *data;     /**< owns the data. */
//    fiff_int_t   *inds;     /**< index list, points into data, no dealloc!. */
//    fiff_int_t   *ptrs;     /**< pointer list, points into data, no dealloc!. */
//} *fiffSparseMatrix, fiffSparseMatrixRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE FIFFLIB

#endif // FIFFSPARSEMATRIX_H
