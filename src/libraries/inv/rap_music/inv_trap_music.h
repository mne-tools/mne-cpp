//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_trap_music.h
 * @since May 2026
 * @brief Truncated RAP-MUSIC (TRAP-MUSIC) source-localisation algorithm — sub-space truncation per iteration for robust correlated-source resolution.
 *
 * @ref INVLIB::InvTrapMusic implements the TRAP-MUSIC algorithm of
 * Mäkelä et al., NeuroImage 197, 616-626 (2019). It extends classic
 * RAP-MUSIC by truncating the signal subspace by one dimension after
 * each found source, which removes the rank-collapse failure mode that
 * RAP-MUSIC exhibits when sources are strongly correlated. The class
 * scans the leadfield for the grid point with the largest subspace
 * correlation, projects it out, truncates the subspace and repeats
 * until the requested source count is reached or the correlation drops
 * below the user threshold. Returns a list of @ref TrapMusicDipole
 * records carrying grid index, correlation, 3-D position and
 * orientation per found dipole.
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
