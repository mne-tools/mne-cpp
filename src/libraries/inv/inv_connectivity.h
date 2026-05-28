//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_connectivity.h
 * @since 2026
 * @date  March 2026
 * @brief Lightweight pairwise source-connectivity container that travels alongside a source estimate.
 *
 * @ref INVLIB::InvConnectivity stores one @c N×N connectivity matrix
 * between source-space grid points together with its measure name
 * (coherence, imCoh, PLV, PLI, wPLI, Granger, PDC, DTF, ...), direction
 * flag and frequency/time window. The struct is deliberately decoupled
 * from the connectivity library so an @ref InvSourceEstimate can carry
 * multiple connectivity layers (one per band / window) without pulling
 * in the full CONNECTIVITY graph machinery.
 */

#ifndef INV_CONNECTIVITY_H
#define INV_CONNECTIVITY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <string>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
/**
 * Stores a pairwise connectivity measure between source locations (grid points, sEEG contacts, etc.).
 * The matrix is [N x N] where N equals the number of sources in the parent InvSourceEstimate.
 * Each InvConnectivity instance represents one metric in one frequency band and time window.
 * For frequency-resolved connectivity, add one entry per band to the parent's connectivity vector.
 *
 * This struct is intentionally lightweight and self-contained — it does not depend on the
 * connectivity library.  Application code can populate it from Network::getFullConnectivityMatrix().
 *
 * @brief Pairwise source connectivity matrix with measure, directionality, and frequency/time metadata.
 */
struct InvConnectivity
{
    Eigen::MatrixXd matrix;     /**< [N x N] connectivity values between sources. */
    std::string     measure;    /**< Metric name, e.g. "coh", "imcoh", "plv", "pli", "wpli", "granger", "pdc", "dtf". */
    bool            directed;   /**< True for directed measures (Granger, PDC, DTF); false for undirected (COH, PLV, PLI). */
    float           fmin;       /**< Lower frequency bound (Hz); 0 if broadband / time-domain. */
    float           fmax;       /**< Upper frequency bound (Hz); 0 if broadband / time-domain. */
    float           tmin;       /**< Start of the time window (s) this connectivity represents. */
    float           tmax;       /**< End of the time window (s) this connectivity represents. */

    InvConnectivity()
        : directed(false)
        , fmin(0.0f)
        , fmax(0.0f)
        , tmin(0.0f)
        , tmax(0.0f)
    {
    }
};

} // NAMESPACE INVLIB

#endif // INV_CONNECTIVITY_H
