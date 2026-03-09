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
 * @brief    MNECovMatrix class declaration.
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

/**
 * Channel-type classification constants for the ch_class field in MNECovMatrix.
 */
#define MNE_COV_CH_UNKNOWN  -1  /**< Unknown channel type. */
#define MNE_COV_CH_MEG_MAG   0  /**< Axial gradiometer or magnetometer [T]. */
#define MNE_COV_CH_MEG_GRAD  1  /**< Planar gradiometer [T/m]. */
#define MNE_COV_CH_EEG       2  /**< EEG [V]. */

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

class MNEProjOp;
class MNESssData;

//=============================================================================================================
/**
 * @brief Covariance matrix storage.
 *
 * Stores a noise or source covariance matrix in dense (packed lower-triangle),
 * diagonal, or sparse form together with its eigendecomposition and associated
 * metadata (projection operator, SSS info, channel classification, bad channels).
 */
class MNESHARED_EXPORT MNECovMatrix
{
public:
    typedef QSharedPointer<MNECovMatrix> SPtr;              /**< Shared pointer type for MNECovMatrix. */
    typedef QSharedPointer<const MNECovMatrix> ConstSPtr;   /**< Const shared pointer type for MNECovMatrix. */

    //=========================================================================================================
    /**
     * Construct a covariance matrix.
     *
     * @param[in] p_kind        Covariance kind (sensor or source).
     * @param[in] p_ncov        Dimension (number of channels).
     * @param[in] p_names       Channel names.
     * @param[in] p_cov         Packed lower-triangle data (may be empty).
     * @param[in] p_cov_diag    Diagonal data (may be empty).
     * @param[in] p_cov_sparse  Sparse covariance data (may be nullptr).
     */
    MNECovMatrix(int p_kind, int p_ncov, const QStringList& p_names, const Eigen::VectorXd& p_cov, const Eigen::VectorXd& p_cov_diag, FIFFLIB::FiffSparseMatrix* p_cov_sparse);

    //=========================================================================================================
    /**
     * Destructor.
     */
    ~MNECovMatrix();

    //=========================================================================================================
    /**
     * Create a deep copy of this covariance matrix including data,
     * channel classes, bad channel list, projection, and SSS info.
     *
     * @return A newly allocated copy.
     */
    std::unique_ptr<MNECovMatrix> dup() const;

    //=========================================================================================================
    /**
     * Create a dense (full lower-triangle packed) covariance matrix.
     *
     * @param[in] kind   Covariance kind (sensor or source).
     * @param[in] ncov   Dimension (number of channels).
     * @param[in] names  Channel names.
     * @param[in] cov    Packed lower-triangle data (length ncov*(ncov+1)/2). Ownership transferred.
     *
     * @return A new covariance matrix.
     */
    static std::unique_ptr<MNECovMatrix> create_dense(int    kind,
                                   int    ncov,
                                   const QStringList& names,
                                   const Eigen::VectorXd& cov)
    {
        return std::unique_ptr<MNECovMatrix>(new MNECovMatrix(kind,ncov,names,cov,Eigen::VectorXd(),nullptr));
    }

    //=========================================================================================================
    /**
     * Create a diagonal covariance matrix.
     *
     * @param[in] kind      Covariance kind (sensor or source).
     * @param[in] ncov      Dimension (number of channels).
     * @param[in] names     Channel names.
     * @param[in] cov_diag  Diagonal data (ncov elements).
     *
     * @return A new covariance matrix.
     */
    static std::unique_ptr<MNECovMatrix> create_diag(int    kind,
                                  int    ncov,
                                  const QStringList& names,
                                  const Eigen::VectorXd& cov_diag)
    {
        return std::unique_ptr<MNECovMatrix>(new MNECovMatrix(kind,ncov,names,Eigen::VectorXd(),cov_diag,nullptr));
    }

    //=========================================================================================================
    /**
     * Create a sparse covariance matrix.
     *
     * @param[in] kind        Covariance kind (sensor or source).
     * @param[in] ncov        Dimension (number of channels).
     * @param[in] names       Channel names.
     * @param[in] cov_sparse  Sparse covariance data (note: data are floats).
     *
     * @return A new covariance matrix.
     */
    static std::unique_ptr<MNECovMatrix> create_sparse(    int kind,
                                                int ncov,
                                                const QStringList& names,
                                                FIFFLIB::FiffSparseMatrix* cov_sparse)
    {
        return std::unique_ptr<MNECovMatrix>(new MNECovMatrix(kind,ncov,names,Eigen::VectorXd(),Eigen::VectorXd(),cov_sparse));
    }

    //=========================================================================================================
    /**
     * Create a covariance matrix from either dense or diagonal data.
     *
     * @param[in] kind      Covariance kind (sensor or source).
     * @param[in] ncov      Dimension (number of channels).
     * @param[in] names     Channel names.
     * @param[in] cov       Packed lower-triangle data (may be empty).
     * @param[in] cov_diag  Diagonal data (may be empty).
     *
     * @return A new covariance matrix.
     */
    static std::unique_ptr<MNECovMatrix> create(   int kind,
                                        int ncov,
                                        const QStringList& names,
                                        const Eigen::VectorXd& cov,
                                        const Eigen::VectorXd& cov_diag)
    {
        return std::unique_ptr<MNECovMatrix>(new MNECovMatrix(kind,ncov,names,cov,cov_diag,nullptr));
    }

    //=========================================================================================================
    /**
     * Check whether this covariance matrix is stored in diagonal form.
     *
     * @return Non-zero if diagonal, zero if full or sparse.
     */
    int is_diag() const;

    //=========================================================================================================
    /**
     * Compute the inverse square-root of eigenvalues (or diagonal elements)
     * for whitening, storing the result in inv_lambda.
     *
     * @return OK on success, FAIL if neither diagonal nor decomposed.
     */
    int add_inv();

    //=========================================================================================================
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

    //=========================================================================================================
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

    //=========================================================================================================
    /**
     * Convenience wrapper for decompose_eigen_small() with default threshold
     * and no rank override.
     *
     * @return OK on success, FAIL on error.
     */
    int decompose_eigen();

private:

    //=========================================================================================================
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
    Eigen::VectorXd cov;                        /**< Packed lower-triangle covariance data (ncov*(ncov+1)/2 elements). */
    Eigen::VectorXd cov_diag;                   /**< Diagonal covariance data (ncov elements). */
    std::unique_ptr<FIFFLIB::FiffSparseMatrix> cov_sparse;   /**< Sparse covariance matrix (note: data are floats). */
    Eigen::VectorXd lambda;                     /**< Eigenvalues of the covariance matrix. */
    Eigen::VectorXd inv_lambda;                 /**< Inverse square-roots of eigenvalues (for whitening). */
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> eigen;  /**< Eigenvectors of the covariance matrix (nzero columns removed). */
    std::unique_ptr<MNEProjOp> proj;            /**< The projection operator active when this matrix was computed. */
    std::unique_ptr<MNESssData> sss;            /**< SSS data from the associated raw data file. */
    Eigen::VectorXi ch_class;                   /**< Per-channel type classification for regularization (MEG grad, MEG mag, EEG). */
    QStringList bads;                           /**< Channel names designated bad during computation. */
    int         nbad;                           /**< Number of bad channels. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNECOVMATRIX_H
