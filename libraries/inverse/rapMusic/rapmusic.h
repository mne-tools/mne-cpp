//=============================================================================================================
/**
 * @file     rapmusic.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    RapMusic algorithm class declaration.
 *
 */

#ifndef RAPMUSIC_H
#define RAPMUSIC_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inverse_global.h"
#include "../IInverseAlgorithm.h"

#include "dipole.h"

#include <mne/mne_forwardsolution.h>
#include <mne/mne_sourceestimate.h>
#include <time.h>

#include <QVector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SVD>
#include <Eigen/LU>

//=============================================================================================================
// DEFINE NAMESPACE INVERSELIB
//=============================================================================================================

namespace INVERSELIB
{

//=============================================================================================================
// SOME DEFINES
//=============================================================================================================

#define NOT_TRANSPOSED   0  /**< Defines NOT_TRANSPOSED. */
#define IS_TRANSPOSED   1   /**< Defines IS_TRANSPOSED. */

//=============================================================================================================
/**
 * Declares a pair structure for index combinations used in RAP MUSIC algorithm.
 */
typedef struct Pair
{
    int x1; /**< Index one of the pair. */
    int x2; /**< Index two of the pair. */
} Pair;

//=============================================================================================================
/**
 * @brief    The RapMusic class provides the RAP MUSIC Algorithm CPU implementation. ToDo: Paper references.
 *
 * ToDo Detailed description
 */
class INVERSESHARED_EXPORT RapMusic : public IInverseAlgorithm
{
public:
    typedef QSharedPointer<RapMusic> SPtr;             /**< Shared pointer type for RapMusic. */
    typedef QSharedPointer<const RapMusic> ConstSPtr;  /**< Const shared pointer type for RapMusic. */

    //*********************************************************************************************************
    //=========================================================================================================
    // TYPEDEFS
    //=========================================================================================================

    typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> MatrixXT;  /**< Defines Eigen::Matrix<T, Eigen::Dynamic,
                                                                             Eigen::Dynamic> as MatrixXT type. */
    typedef Eigen::Matrix<double, Eigen::Dynamic, 6> MatrixX6T;              /**< Defines Eigen::Matrix<T, Eigen::Dynamic,
                                                                             6> as MatrixX6T type. */
    typedef Eigen::Matrix<double, 6, Eigen::Dynamic> Matrix6XT;              /**< Defines Eigen::Matrix<T, 6,
                                                                             Eigen::Dynamic> as Matrix6XT type. */
    typedef Eigen::Matrix<double, 6, 6> Matrix6T;                            /**< Defines Eigen::Matrix<T, 6, 6>
                                                                             as Matrix6T type. */
    typedef Eigen::Matrix<double, Eigen::Dynamic, 1> VectorXT;               /**< Defines Eigen::Matrix<T, Eigen::Dynamic,
                                                                             1> as VectorXT type. */
    typedef Eigen::Matrix<double, 6, 1> Vector6T;                            /**< Defines Eigen::Matrix<T, 6, 1>
                                                                             as Vector6T type. */

    //=========================================================================================================
    /**
     * Default constructor creates an empty RapMusic algorithm which still needs to be initialized.
     */
    RapMusic();

    //=========================================================================================================
    /**
     * Constructor which initializes the RapMusic algorithm with the given model.
     *
     * @param[in] p_Fwd          The model which contains the gain matrix and its corresponding grid matrix.
     * @param[in] p_bSparsed     True when sparse matrices should be used.
     * @param[in] p_iN           The number (default 2) of uncorrelated sources, which should be found. Starting with.
     *                           the strongest.
     * @param[in] p_dThr         The correlation threshold (default 0.5) at which the search for sources stops.
     */
    RapMusic(MNELIB::MNEForwardSolution& p_pFwd, bool p_bSparsed, int p_iN = 2, double p_dThr = 0.5);

    virtual ~RapMusic();

    //=========================================================================================================
    /**
     * Initializes the RAP MUSIC algorithm with the given model.
     *
     * @param[in] p_Fwd          The model which contains the gain matrix and its corresponding Grid matrix.
     * @param[in] p_bSparsed     True when sparse matrices should be used.
     * @param[in] p_iN           The number (default 2) of uncorrelated sources, which should be found. Starting with.
     *                           the strongest.
     * @param[in] p_dThr         The correlation threshold (default 0.5) at which the search for sources stops.
     * @return   true if successful initialized, false otherwise.
     */
    bool init(MNELIB::MNEForwardSolution& p_pFwd, bool p_bSparsed = false, int p_iN = 2, double p_dThr = 0.5);

    virtual MNELIB::MNESourceEstimate calculateInverse(const FIFFLIB::FiffEvoked &p_fiffEvoked, bool pick_normal = false);

    virtual MNELIB::MNESourceEstimate calculateInverse(const Eigen::MatrixXd &data, float tmin, float tstep, bool pick_normal = false) const;

    virtual MNELIB::MNESourceEstimate calculateInverse(const Eigen::MatrixXd& p_matMeasurement, QList< DipolePair<double> > &p_RapDipoles) const;

    virtual const char* getName() const;

    virtual const MNELIB::MNESourceSpace& getSourceSpace() const;

    //=========================================================================================================
    /**
     * Sets the source estimate attributes.
     *
     * @param[in] p_iSampStcWin  Samples per source localization window (default - 1 = not set).
     * @param[in] p_fStcOverlap  Percentage of localization window overlap.
     */
    void setStcAttr(int p_iSampStcWin, float p_fStcOverlap);

protected:
    //=========================================================================================================
    /**
     * Computes the signal subspace Phi_s out of the measurement F.
     *
     * @param[in] p_pMatMeasurement  The current measured data to process (for best performance it should have.
                                    the dimension channels x samples with samples = number of channels)
     * @param[out] p_pMatPhi_s   The calculated signal subspace.
     * @return   The rank of the measurement F (named r lt. Mosher 1998, 1999).
     */
    int calcPhi_s(const MatrixXT& p_matMeasurement, MatrixXT* &p_pMatPhi_s) const;

    //=========================================================================================================
    /**
     * Computes the subspace correlation between the projected G_rho and the projected signal subspace Phi_s.
     * For speed-up: we calculate the decomposition of the projected Phi_s before this function. So the argument
     * for this function is U_B instead of Phi_s.
     *
     * @param[in] p_matProj_G    The projected Lead Field combination. This is a m x 6 matrix composed of the.
     *                           Lead Field combination of two Points for all m channels and 3 orthogonal
     *                           components (x y z).
     * @param[in] p_matU_B       The matrix U is the subspace projection of the orthogonal projected Phi_s.
     * @return   The maximal correlation c_1 of the subspace correlation of the current projected Lead Field.
     *           combination and the projected measurement.
     */
    static double subcorr(MatrixX6T& p_matProj_G, const MatrixXT& p_pMatU_B);

    //=========================================================================================================
    /**
     * Computes the subspace correlation between the projected G_rho and the projected signal subspace Phi_s, as
     * well as the resulting direction.
     * For speed-up: we calculate the decomposition of the projected Phi_s before this function. So the argument
     * for this function is U_B instead of Phi_s.
     *
     * @param[in] p_matProj_G    The projected Lead Field combination. This is a m x 6 matrix composed of the.
     *                           Lead Field combination of two Points for all m channels and 3 orthogonal
     *                           components (x y z).
     * @param[in] p_matU_B    The matrix U is the subspace projection of the orthogonal projected Phi_s.
     * @param[out] p_vec_phi_k_1 Returns the orientation for a correlated dipole pair.
                                (phi_x1, phi_y1, phi_z1, phi_x2, phi_y2, phi_z2)
     * @return   The maximal correlation c_1 of the subspace correlation of the current projected Lead Field.
     *           combination and the projected measurement.
     */
    static double subcorr(MatrixX6T& p_matProj_G, const MatrixXT& p_matU_B, Vector6T& p_vec_phi_k_1);

    //=========================================================================================================
    /**
     * Calculates the accumulated manifold vectors A_{k1}
     *
     * @param[in] p_matG_k_1 The Lead Field combination for the currently best correlated pair.
     * @param[in] p_matPhi_k_1   Is equal to u_k_1 in the paper and it is the direction of the currently best.
     *                           correlated pair.
     * @param[in] p_iIdxk_1  The current position in the manifold vector array A_k_1.
     * @param[out] p_matA_k_1    The array of the manifold vectors.
     */
    static void calcA_k_1(  const MatrixX6T& p_matG_k_1,
                            const Vector6T& p_matPhi_k_1,
                            const int p_iIdxk_1,
                            MatrixXT& p_matA_k_1);

    //=========================================================================================================
    /**
     * Calculates the orthogonal projector Phi_A_k_1 like in the paper Mosher 1999 (13)
     *
     * @param[in] p_matA_k_1 The array of the manifold vectors.
     * @param[out] p_matOrthProj The orthogonal projector.
     */
    void calcOrthProj(const MatrixXT& p_matA_k_1, MatrixXT& p_matOrthProj) const;

    //=========================================================================================================
    /**
     * Pre-Calculates the gain matrix index combinations to search for a two dipole independent topography
     * (IT = source).
     *
     * @param[in] p_iNumPoints   The number of Lead Field points -> for dimension check.
     * @param[in] p_iNumCombinations The number of pair index combinations.
     * @param[out] p_ppPairIdxCombinations   The destination which contains pointer to pointer of index.
     *                                       combinations of Lead Field indices -> Number of pointers =
     *                                       Combination (number of grid points over 2 = Num + 1 C 2)
     */
    void calcPairCombinations(  const int p_iNumPoints,
                                const int p_iNumCombinations,
                                Pair** p_ppPairIdxCombinations) const;

    //=========================================================================================================
    /**
     * Calculates the combination indices Idx1 and Idx2 of n points.\n
     *   [  (0,0)   (0,1)   (0,2)  ...  (0,n-1)   (0,n)\n
     *              (1,1)   (1,2)  ...  (1,n-1)   (1,n)\n
     *                      (2,2)  ...  (2,n-1)   (2,n)\n
     *\n
     *                                  (n-1,n-1) (n-1,n)\n
     *                                            (n,n)]
     *
     *
     * @param[in] p_iPoints  The number of points n which are combined with each other.
     * @param[in] p_iCurIdx  The current combination index (between 0 and nchoosek(n+1,2)).
     * @param[out] p_iIdx1   The resulting index 1.
     * @param[out] p_iIdx2   The resulting index 2.
     */
    static void getPointPair(const int p_iPoints, const int p_iCurIdx, int &p_iIdx1, int &p_iIdx2);

    //=========================================================================================================
    /**
     * Returns a gain matrix pair for the given indices
     *
     * @param[in]   p_matGainMarix The Lead Field matrix.
     * @param[out]   p_matGainMarix_Pair   Lead Field combination (dimension: m x 6).
     * @param[in]   p_iIdx1 first Lead Field index point.
     * @param[in]   p_iIdx2 second Lead Field index point.
     */
    static void getGainMatrixPair(  const MatrixXT& p_matGainMarix,
                                    MatrixX6T& p_matGainMarix_Pair,
                                    int p_iIdx1, int p_iIdx2);

    //=========================================================================================================
    /**
     * Adds a new correlated dipole pair to th RapDipoles. This function is called by the RAP MUSIC Algorithm.
     *
     * @param[in] p_iDipoleIdx1  Index (Lead Field grid index) of the first dipole.
     * @param[in] p_iDipoleIdx2  Index (Lead Field grid index) of the second dipole.
     * @param[in] p_vec_phi_k_1  Array of the dipole directories (phi_x1, phi_y1, phi_z1, phi_x2, phi_y2, phi_z2).
     * @param[in] p_valCor       Correlation value of the dipole pair.
     * @param[out] p_RapDipoles  the list of dipole pairs.
     */
    static void insertSource(  int p_iDipoleIdx1, int p_iDipoleIdx2,
                        const Vector6T &p_vec_phi_k_1,
                        double p_valCor,
                        QList< DipolePair<double> > &p_RapDipoles);

    MNELIB::MNEForwardSolution m_ForwardSolution; /**< The Forward operator which should be scanned through*/

    int m_iN;               /**< Number of Sources to find*/
    double m_dThreshold;    /**< Threshold which defines the minimal correlation. Is the correlation of
                                 the found dipole pair smaller as this threshold than the RAP MUSIC
                                 calculation is stopped. */

    int m_iNumGridPoints;               /**< Number of Grid points. */
    int m_iNumChannels;                 /**< Number of channels. */
    int m_iNumLeadFieldCombinations;    /**< Number of Lead Filed combinations (grid points + 1 over 2)*/

    Pair** m_ppPairIdxCombinations; /**< Index combination vector with grid pair indices. */

    int m_iMaxNumThreads;   /**< Number of available CPU threads. */

    bool m_bIsInit; /**< Whether the algorithm is initialized. */

    //Stc stuff
    int m_iSamplesStcWindow;    /**< Number of samples per localization window. */
    float m_fStcOverlap;        /**< Percentage of localization window overlap. */

    //=========================================================================================================
    /**
     * Returns the rank r of a singular value matrix based on non-zero singular values
     * (singular value > epsilon = 10^-5)
     *
     * @param[in] p_matSigma diagonal matrix which contains the Singular values (Dimension n x n).
     * @return The rank r.
     */
    static inline int getRank(const MatrixXT& p_matSigma);

    //=========================================================================================================
    /**
     * lt. Mosher 1998 -> Only Retain those Components of U_A and U_B that correspond to nonzero singular values
     * for U_A and U_B the number of columns corresponds to their ranks
     *
     * @param[in] p_Mat  The Matrix which should be reduced to its rank.
     * @param[in] p_matSigma_src The singular values of the matrix.
     * @param[out] p_matFull_Rank    The corresponding full rank matrix.
     * @param[in] type   Whether p_Mat is transposed, than rows and columns are changed.
     */
    static inline int useFullRank( const MatrixXT& p_Mat,
                            const MatrixXT& p_matSigma_src,
                            MatrixXT& p_matFull_Rank,
                            int type = NOT_TRANSPOSED);

    //=========================================================================================================
    /**
     * Performs F * F^Transposed, is used when n > m
     *
     * @param[in] p_matF The matrix which should be transformed.
     * @return F * F^Transposed (we call it FFT ;)).
     */
    static inline MatrixXT makeSquareMat(const MatrixXT& p_matF);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline int RapMusic::getRank(const MatrixXT& p_matSigma)
{
    int t_iRank;
    //if once a singularvalue is smaller than epsilon = 10^-5 the following values are also smaller
    // -> because Singular values are ordered
    for(t_iRank = p_matSigma.rows()-1; t_iRank > 0; t_iRank--)
        if (p_matSigma(t_iRank, t_iRank) > 0.00001)
            break;

    t_iRank++;//rank corresponding to epsilon

    return t_iRank;
}

//=============================================================================================================

inline int RapMusic::useFullRank(   const MatrixXT& p_Mat,
                                    const MatrixXT& p_matSigma_src,
                                    MatrixXT& p_matFull_Rank,
                                    int type)
{
    int rank = getRank(p_matSigma_src);

    if (type == NOT_TRANSPOSED)
        p_matFull_Rank = p_Mat.block(0,0,p_Mat.rows(),rank);
    else
        p_matFull_Rank = p_Mat.block(0,0,rank,p_Mat.cols());

    return rank;
}

//=============================================================================================================

inline RapMusic::MatrixXT RapMusic::makeSquareMat(const MatrixXT& p_matF)
{
    //Make rectangular - p_matF*p_matF^T
    //MatrixXT FFT = p_matF*p_matF.transpose();

    MatrixXT mat = p_matF.transpose();

    return p_matF*mat;
}
} //NAMESPACE

#endif // RAPMUSIC_H
