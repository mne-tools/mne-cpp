//=============================================================================================================
/**
 * @file     mne_corsourceestimate.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     November, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief     MNECorSourceEstimate class declaration.
 *
 */

#ifndef MNECORSOURCEESTIMATE_H
#define MNECORSOURCEESTIMATE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_sourceestimate.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/SparseCore>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>
#include <QIODevice>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Correlated source estimation which holds results of MNE-CPP inverse routines, which estimate the correlation too
 *
 * @brief Correlated source estimation
 */
class MNESHARED_EXPORT MNECorSourceEstimate : public MNESourceEstimate
{
public:

    //=========================================================================================================
    /**
     * Default constructor
     */
    MNECorSourceEstimate();

    //=========================================================================================================
    /**
     * Constructs a source estimation from given data
     *
     * @param[in] p_sol.
     * @param[in] p_vertices.
     * @param[in] p_tmin.
     * @param[in] p_tstep.
     */
    MNECorSourceEstimate(const Eigen::MatrixXd &p_sol, const Eigen::VectorXi &p_vertices, float p_tmin, float p_tstep);

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_SourceEstimate    Source estimate data which should be copied.
     */
    MNECorSourceEstimate(const MNECorSourceEstimate& p_SourceEstimate);

    //=========================================================================================================
    /**
     * Constructs a source estimation, by reading from a IO device.
     *
     * @param[in] p_IODevice     IO device to read from the source estimation.
     *
     */
    MNECorSourceEstimate(QIODevice &p_IODevice);

    //=========================================================================================================
    /**
     * Initializes source estimate.
     */
    void clear();

    //=========================================================================================================
    /**
     * mne_read_stc_file
     *
     * Reads a source estimate from a given file
     *
     * @param[in] p_IODevice    IO device to red the stc from.
     * @param[in, out] p_stc        the read stc.
     *
     * @return true if successful, false otherwise.
     */
    static bool read(QIODevice &p_IODevice, MNECorSourceEstimate& p_stc);

    //=========================================================================================================
    /**
     * mne_write_stc_file
     *
     * Writes a stc file
     *
     * @param[in] p_IODevice   IO device to write the stc to.
     */
    bool write(QIODevice &p_IODevice);

    //=========================================================================================================
    /**
     * Assignment Operator
     *
     * @param[in] rhs     SourceEstimate which should be assigned.
     *
     * @return the copied source estimate.
     */
    MNECorSourceEstimate& operator= (const MNECorSourceEstimate &rhs);

private:
    Eigen::SparseMatrix<float> m_matCorrelations;  /**< Upper triangular matrix of shape [n_dipoles x n_dipoles] which contains the dipole correlations. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} //NAMESPACE

#endif // MNECORSOURCEESTIMATE_H
