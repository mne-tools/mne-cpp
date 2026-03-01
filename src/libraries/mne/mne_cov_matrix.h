//=============================================================================================================
/**
 * @file     mne_cov_matrix.h
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
 * @brief    MneCovMatrix class declaration.
 *
 */

#ifndef MNECOVMATRIX_H
#define MNECOVMATRIX_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QStringList>

/*
 * The class field in mneCovMatrix can have these values
 */
#define MNE_COV_CH_UNKNOWN  -1	/* No idea */
#define MNE_COV_CH_MEG_MAG   0  /* Axial gradiometer or magnetometer [T] */
#define MNE_COV_CH_MEG_GRAD  1  /* Planar gradiometer [T/m] */
#define MNE_COV_CH_EEG       2  /* EEG [V] */

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB
{
    class FiffSparseMatrix;
}

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// MNELIB FORWARD DECLARATIONS
//=============================================================================================================

class MneProjOp;
class MneSssData;

//=============================================================================================================
/**
 * Implements an MNE Covariance Matrix (Replaces *mneCovMatrix,mneCovMatrixRec; struct of MNE-C mne_types.h).
 *
 * @brief Covariance matrix storage
 */
class MNESHARED_EXPORT MneCovMatrix
{
public:
    typedef QSharedPointer<MneCovMatrix> SPtr;              /**< Shared pointer type for MneCovMatrix. */
    typedef QSharedPointer<const MneCovMatrix> ConstSPtr;   /**< Const shared pointer type for MneCovMatrix. */

    //=========================================================================================================
    /**
     * Constructs the MNE Covariance Matrix
     * Refactored: new_cov (mne_cov_matrix.c)
     */
    MneCovMatrix(int p_kind, int p_ncov, const QStringList& p_names, double *p_cov, double *p_cov_diag, FIFFLIB::FiffSparseMatrix* p_cov_sparse);

    //=========================================================================================================
    /**
     * Destroys the MNE Covariance Matrix
     * Refactored: mne_free_cov (mne_cov_matrix.c)
     */
    ~MneCovMatrix();

    /**
     * Create a deep copy of this covariance matrix including data,
     * channel classes, bad channel list, projection, and SSS info.
     *
     * @return A newly allocated copy. Caller takes ownership.
     */
    MneCovMatrix* dup() const;

    /**
     * Create a dense (full lower-triangle packed) covariance matrix.
     *
     * @param[in] kind   Covariance kind (sensor or source).
     * @param[in] ncov   Dimension (number of channels).
     * @param[in] names  Channel names.
     * @param[in] cov    Packed lower-triangle data (length ncov*(ncov+1)/2). Ownership transferred.
     *
     * @return A new covariance matrix. Caller takes ownership.
     */
    static MneCovMatrix* create_dense(int    kind,
                                   int    ncov,
                                   const QStringList& names,
                                   double *cov)
    {
        return new MneCovMatrix(kind,ncov,names,cov,NULL,NULL);
    }

    static MneCovMatrix* create_diag(int    kind,
                                  int    ncov,
                                  const QStringList& names,
                                  double *cov_diag)
    {
        return new MneCovMatrix(kind,ncov,names,NULL,cov_diag,NULL);
    }

    static MneCovMatrix* create_sparse(    int kind,
                                                int ncov,
                                                const QStringList& names,
                                                FIFFLIB::FiffSparseMatrix* cov_sparse)
    {
        return new MneCovMatrix(kind,ncov,names,NULL,NULL,cov_sparse);
    }

    /**
     * Create a covariance matrix from either dense or diagonal data.
     *
     * @param[in] kind      Covariance kind (sensor or source).
     * @param[in] ncov      Dimension (number of channels).
     * @param[in] names     Channel names.
     * @param[in] cov       Packed lower-triangle data (may be NULL).
     * @param[in] cov_diag  Diagonal data (may be NULL).
     *
     * @return A new covariance matrix. Caller takes ownership.
     */
    static MneCovMatrix* create(   int kind,
                                        int ncov,
                                        const QStringList& names,
                                        double      *cov,
                                        double *cov_diag)
    {
        return new MneCovMatrix(kind,ncov,names,cov,cov_diag,NULL);
    }

    /**
     * Check whether this covariance matrix is stored in diagonal form.
     *
     * @return Non-zero if diagonal, zero if full or sparse.
     */
    int is_diag() const;

    /**
     * Compute the inverse square-root of eigenvalues (or diagonal elements)
     * for whitening, storing the result in inv_lambda.
     *
     * @return OK on success, FAIL if neither diagonal nor decomposed.
     */
    int add_inv();

    /**
     * Condition the covariance matrix by eigendecomposition with per-channel-type
     * scaling, zeroing sub-threshold eigenvalues, and reconstructing.
     *
     * @param[in] rank_threshold  Eigenvalue threshold ratio for rank estimation.
     * @param[in] use_rank        If positive, override the automatic rank estimate.
     *
     * @return The estimated rank, or FAIL on error.
     */
    int condition(float rank_threshold, int use_rank);

    /**
     * Perform eigenvalue decomposition of the covariance matrix, zero
     * sub-threshold eigenvalues, classify eigenvectors by channel type,
     * and compute inverse square-roots for whitening.
     *
     * @param[in] p_small   Eigenvalue threshold (negative uses default).
     * @param[in] use_rank  If positive, override automatic rank estimate.
     *
     * @return OK on success, FAIL on error.
     */
    int decompose_eigen_small(float p_small, int use_rank);

    /**
     * Convenience wrapper for decompose_eigen_small() with default threshold
     * and no rank override.
     *
     * @return OK on success, FAIL on error.
     */
    int decompose_eigen();

private:

    /**
     * Compute the linear index into a symmetric lower-triangular packed
     * storage array for element (j, k).
     *
     * @param[in] j   Row index.
     * @param[in] k   Column index.
     *
     * @return The packed storage index.
     */
    static int lt_packed_index(int j, int k);

public:
    int         kind;                           /**< Covariance kind: sensor or source. */
    int         ncov;                           /**< Dimension (number of channels). */
    int         nfree;                          /**< Number of degrees of freedom used in estimation. */
    int         nproj;                          /**< Number of dimensions projected out. */
    int         nzero;                          /**< Number of zero or small eigenvalues. */
    QStringList names;                          /**< Channel names (optional). */
    double      *cov;                           /**< Packed lower-triangle covariance data (ncov*(ncov+1)/2 elements). */
    double      *cov_diag;                      /**< Diagonal covariance data (ncov elements). */
    std::unique_ptr<FIFFLIB::FiffSparseMatrix> cov_sparse;   /**< Sparse covariance matrix (note: data are floats). */
    double      *lambda;                        /**< Eigenvalues of the covariance matrix. */
    double      *inv_lambda;                    /**< Inverse square-roots of eigenvalues (for whitening). */
    float       **eigen;                        /**< Eigenvectors of the covariance matrix (nzero columns removed). */
    double      *chol;                          /**< Cholesky decomposition of the covariance matrix. */
    std::unique_ptr<MneProjOp> proj;            /**< The projection operator active when this matrix was computed. */
    std::unique_ptr<MneSssData> sss;            /**< SSS data from the associated raw data file. */
    int         *ch_class;                      /**< Per-channel type classification for regularization (MEG grad, MEG mag, EEG). */
    QStringList bads;                           /**< Channel names designated bad during computation. */
    int         nbad;                           /**< Number of bad channels. */

// ### OLD STRUCT ###
//typedef struct {                                /* Covariance matrix storage */
//    int         kind;                           /* Sensor or source covariance */
//    int         ncov;                           /* Dimension */
//    int         nfree;                          /* Number of degrees of freedom */
//    int         nproj;                          /* Number of dimensions projected out */
//    int         nzero;                          /* Number of zero or small eigenvalues */
//    char        **names;                        /* Names of the entries (optional) */
//    double      *cov;                           /* Covariance matrix in packed representation (lower triangle) */
//    double      *cov_diag;                      /* Diagonal covariance matrix */
//    MNELIB::FiffSparseMatrix* cov_sparse;   /* A sparse covariance matrix (Note: data are floats in this which is an inconsistency) */
//    double      *lambda;                /* Eigenvalues of cov */
//    double      *inv_lambda;            /* Inverses of the square roots of the eigenvalues of cov */
//    float       **eigen;                /* Eigenvectors of cov */
//    double      *chol;                  /* Cholesky decomposition */
//    MNELIB::MneProjOp*  proj;       /* The projection which was active when this matrix was computed */
//    MNELIB::MneSssData* sss;        /* The SSS data present in the associated raw data file */
//    int         *ch_class;              /* This will allow grouping of channels for regularization (MEG [T/m], MEG [T], EEG [V] */
//    char        **bads;                 /* Which channels were designated bad when this noise covariance matrix was computed? */
//    int         nbad;                   /* How many of them */
//} *mneCovMatrix,mneCovMatrixRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNECOVMATRIX_H
