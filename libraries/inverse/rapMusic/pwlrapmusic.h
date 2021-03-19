//=============================================================================================================
/**
 * @file     pwlrapmusic.h
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
 * @brief    PwlRapMusic algorithm class declaration.
 *
 */

#ifndef PWLRAPMUSIC_H
#define PWLRAPMUSIC_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inverse_global.h"
#include "rapmusic.h"

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
 * @brief    The PwlRapMusic class provides the POWELL RAP MUSIC Algorithm CPU implementation. ToDo: Paper references.
 *
 * ToDo Detailed description
 */
class INVERSESHARED_EXPORT PwlRapMusic : public RapMusic
{
public:

    //=========================================================================================================
    /**
     * Default constructor creates an empty POWELL RAP MUSIC algorithm which still needs to be initialized.
     */
    PwlRapMusic();

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
    PwlRapMusic(MNELIB::MNEForwardSolution& p_pFwd, bool p_bSparsed, int p_iN = 2, double p_dThr = 0.5);

    virtual ~PwlRapMusic();

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
    virtual MNELIB::MNESourceEstimate calculateInverse(const FIFFLIB::FiffEvoked &p_fiffEvoked, bool pick_normal = false);

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
    virtual MNELIB::MNESourceEstimate calculateInverse(const Eigen::MatrixXd &data, float tmin, float tstep) const;

    virtual MNELIB::MNESourceEstimate calculateInverse(const Eigen::MatrixXd& p_matMeasurement, QList< DipolePair<double> > &p_RapDipoles) const;

    static int PowellOffset(int p_iRow, int p_iNumPoints);

    static void PowellIdxVec(int p_iRow, int p_iNumPoints, Eigen::VectorXi& p_pVecElements);

    virtual const char* getName() const;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} //NAMESPACE

#endif // PWLRAPMUSIC_H
