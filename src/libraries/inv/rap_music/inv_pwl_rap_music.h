//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_pwl_rap_music.h
 * @since March 2026
 * @brief Powell-accelerated RAP-MUSIC variant — replaces the exhaustive pair scan with a Powell line-search refinement.
 *
 * @ref INVLIB::InvPwlRapMusic derives from @ref InvRapMusic and
 * overrides the pair-scanning step with a Powell direction-set
 * optimiser, drastically reducing the number of leadfield evaluations
 * per iteration on dense grids. All other behaviour — signal-subspace
 * estimation, recursive projection, stopping criterion, output
 * assembly — is inherited from the base class so results stay
 * comparable to the exhaustive scanner.
 */

#ifndef INV_PWL_RAP_MUSIC_H
#define INV_PWL_RAP_MUSIC_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inv_global.h"
#include "inv_rap_music.h"

#include "inv_dipole.h"

#include <mne/mne_forward_solution.h>
#include <inv/inv_source_estimate.h>
#include <time.h>

#include <QVector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SVD>
#include <Eigen/LU>

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
// SOME DEFINES
//=============================================================================================================

#define NOT_TRANSPOSED   0  /**< Defines NOT_TRANSPOSED. */
#define IS_TRANSPOSED   1   /**< Defines IS_TRANSPOSED. */

//=============================================================================================================
/**
 * @brief Powell-accelerated RAP MUSIC variant using gradient-free optimization for refined dipole fitting
 *
 * ToDo Detailed description
 */
class INVSHARED_EXPORT InvPwlRapMusic : public InvRapMusic
{
public:

    //=========================================================================================================
    /**
     * Default constructor creates an empty POWELL RAP MUSIC algorithm which still needs to be initialized.
     */
    InvPwlRapMusic();

    //=========================================================================================================
    /**
     * Constructor which initializes the POWELL RAP MUSIC algorithm with the given model.
     *
     * @param[in] p_Fwd          The model which contains the gain matrix and its corresponding grid matrix.
     * @param[in] p_bSparsed     True when sparse matrices should be used.
     * @param[in] p_iN           The number (default 2) of uncorrelated sources, which should be found. Starting with.
     *                           the strongest.
     * @param[in] p_dThr         The correlation threshold (default 0.5) at which the search for sources stops.
     */
    InvPwlRapMusic(MNELIB::MNEForwardSolution& p_pFwd, bool p_bSparsed, int p_iN = 2, double p_dThr = 0.5);

    virtual ~InvPwlRapMusic();

    //=========================================================================================================
    /**
     *
     * Note: Since they are virtual they have to be implemented to be called. Even so the base class RAP MUSIC
     *       implementation is called.
     *
     * @param[in] p_fiffEvoked.
     * @param[in] pick_normal.
     *
     * @return
     */
    virtual InvSourceEstimate calculateInverse(const FIFFLIB::FiffEvoked &p_fiffEvoked, bool pick_normal = false);

    //=========================================================================================================
    /**
     *
     * Note: Since they are virtual they have to be implemented to be called. Even so the base class RAP MUSIC
     *       implementation is called.
     *
     * @param[in] data.
     * @param[in] tmin.
     * @param[in] tstep.
     *
     * @return
     */
    virtual InvSourceEstimate calculateInverse(const Eigen::MatrixXd &data, float tmin, float tstep) const;

    virtual InvSourceEstimate calculateInverse(const Eigen::MatrixXd& p_matMeasurement, QList< InvDipolePair<double> > &p_RapDipoles) const;

    static int PowellOffset(int p_iRow, int p_iNumPoints);

    static void PowellIdxVec(int p_iRow, int p_iNumPoints, Eigen::VectorXi& p_pVecElements);

    virtual const char* getName() const;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} //NAMESPACE

#endif // INV_PWL_RAP_MUSIC_H
