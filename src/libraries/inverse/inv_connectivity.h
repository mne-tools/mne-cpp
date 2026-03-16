//=============================================================================================================
/**
 * @file     inv_connectivity.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     June, 2025
 *
 * @section  LICENSE
 *
 * Copyright (C) 2025, Christoph Dinh. All rights reserved.
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
 * @brief    InvConnectivity struct for pairwise source connectivity results.
 *
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
