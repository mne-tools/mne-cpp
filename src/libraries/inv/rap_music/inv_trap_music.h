//=============================================================================================================
/**
 * @file     inv_trap_music.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    TRAP-MUSIC source localization algorithm.
 *
 * Reference: Makela et al., NeuroImage 197, 616-626, 2019.
 */

#ifndef INV_TRAP_MUSIC_H
#define INV_TRAP_MUSIC_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inv_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
/**
 * @brief Result of a TRAP-MUSIC source scan.
 */
struct INVSHARED_EXPORT TrapMusicDipole
{
    int     sourceIdx = -1;      /**< Index in the lead field grid. */
    double  correlation = 0.0;   /**< Subspace correlation value (0..1). */
    Eigen::Vector3d position;    /**< 3D position (meters). */
    Eigen::Vector3d orientation; /**< Dipole orientation (unit vector). */
};

//=============================================================================================================
/**
 * @brief TRAP-MUSIC (Truncated RAP-MUSIC) source localization.
 *
 * Extends RAP-MUSIC by adding a truncation step to the signal subspace
 * at each iteration, improving robustness to correlated sources.
 * Uses SVD of the measurement data to estimate the signal subspace,
 * then iteratively scans the lead field for the best-matching dipole,
 * projects it out, and truncates the subspace dimension.
 *
 * Reference: Makela et al., NeuroImage 197, 616-626, 2019.
 */
class INVSHARED_EXPORT InvTrapMusic
{
public:
    //=========================================================================================================
    /**
     * @brief Construct TRAP-MUSIC scanner.
     *
     * @param[in] iMaxSources   Maximum number of sources to find (default 5).
     * @param[in] dThreshold    Correlation threshold to stop scanning (default 0.85).
     */
    explicit InvTrapMusic(int iMaxSources = 5, double dThreshold = 0.85);

    //=========================================================================================================
    /**
     * @brief Compute TRAP-MUSIC source localization.
     *
     * @param[in] matLeadField  Lead field matrix (n_channels × n_sources*n_orient).
     * @param[in] matData       Measurement data (n_channels × n_times).
     * @param[in] matSourcePos  Source positions (n_sources × 3).
     * @param[in] iNOrient      Number of orientations per source (1=fixed, 3=free; default 3).
     *
     * @return List of found dipoles, ordered by descending correlation.
     */
    QList<TrapMusicDipole> compute(const Eigen::MatrixXd& matLeadField,
                                    const Eigen::MatrixXd& matData,
                                    const Eigen::MatrixXd& matSourcePos,
                                    int iNOrient = 3) const;

    //=========================================================================================================
    /**
     * @brief Compute the MUSIC-type subspace correlation for all source locations.
     *
     * @param[in] matLeadField      Lead field matrix (n_channels × n_sources*n_orient).
     * @param[in] matSignalSubspace Signal subspace (n_channels × n_signal_dims).
     * @param[in] iNOrient          Number of orientations per source (1 or 3).
     *
     * @return Correlation vector (n_sources).
     */
    static Eigen::VectorXd scanCorrelations(const Eigen::MatrixXd& matLeadField,
                                             const Eigen::MatrixXd& matSignalSubspace,
                                             int iNOrient);

private:
    int     m_iMaxSources;
    double  m_dThreshold;
};

} // namespace INVLIB

#endif // INV_TRAP_MUSIC_H
