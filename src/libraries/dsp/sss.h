//=============================================================================================================
/**
 * @file     sss.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
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
 * @brief    Declaration of the SSS class implementing Signal Space Separation (SSS) and
 *           temporal Signal Space Separation (tSSS) for MEG data.
 *
 * Theory:
 *   Taulu, S., Kajola, M. (2005). "Presentation of electromagnetic multichannel data: The signal
 *   space separation method." J. Appl. Phys. 97, 124905.
 *
 *   Taulu, S., Simola, J. (2006). "Spatiotemporal signal space separation method for rejecting
 *   nearby interference in MEG measurements." Phys. Med. Biol. 51, 1759–1768.
 *
 * SSS separates the MEG signal into contributions from internal sources (brain) and external
 * sources (environmental noise) by decomposing sensor data into a basis of spherical harmonics.
 * tSSS additionally removes internal-space components that correlate with the external subspace
 * in sliding time windows, suppressing near-field artefacts (e.g. implants, dental work).
 *
 * The spherical harmonic basis uses 4π-normalised real harmonics.  Integration over each coil
 * is approximated by a single-point model using the coil-centre position and normal direction
 * from FiffChInfo (suitable for standard MEG; for sub-millimetre accuracy use FwdCoilSet).
 */

#ifndef SSS_DSP_H
#define SSS_DSP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"

#include <fiff/fiff_info.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * @brief Implements Signal Space Separation (SSS) and temporal SSS (tSSS) for MEG data.
 *
 * Quick-start example:
 * @code
 *   // Build the SSS basis once from sensor geometry
 *   SSS::Params p;
 *   p.iOrderIn  = 8;                          // Standard internal order
 *   p.iOrderOut = 3;                          // Standard external order
 *   p.origin    = Eigen::Vector3d(0,0,0.04);  // 4 cm above head origin
 *   SSS::Basis basis = SSS::computeBasis(fiffInfo, p);
 *
 *   // Apply SSS (suppress environmental noise)
 *   Eigen::MatrixXd cleanData = SSS::apply(rawMegData, basis);
 *
 *   // Apply tSSS (additionally suppress near-field artefacts)
 *   Eigen::MatrixXd tSssData  = SSS::applyTemporal(rawMegData, basis);
 * @endcode
 */
/** @brief Configuration parameters for SSS/tSSS (defined outside class to work around a Clang default-argument/nested-struct limitation). */
struct DSPSHARED_EXPORT SSSParams
{
    int    iOrderIn  = 8;                      /**< Internal spherical-harmonic expansion order (default 8). N_in = iOrderIn*(iOrderIn+2) = 80. */
    int    iOrderOut = 3;                      /**< External spherical-harmonic expansion order (default 3). N_out = iOrderOut*(iOrderOut+2) = 15. */
    Eigen::Vector3d origin{0.0, 0.0, 0.04};   /**< Expansion origin in metres, head-coordinate frame (default: 4 cm superior to head origin). */
    double dRegIn  = 1e-5;                     /**< Tikhonov regularisation for the combined-basis pseudoinverse. */
};

class DSPSHARED_EXPORT SSS
{
public:
    using Params = SSSParams; /**< Convenience alias so callers can still write SSS::Params. */

    //=========================================================================================================
    /**
     * @brief Precomputed SSS basis and projectors for a given sensor array.
     *
     * Call computeBasis() once; then re-use across many data segments.
     */
    struct Basis
    {
        Eigen::MatrixXd matSin;           /**< Internal basis S_in   (n_meg × N_in). */
        Eigen::MatrixXd matSout;          /**< External basis S_out  (n_meg × N_out). */
        Eigen::MatrixXd matProjIn;        /**< Internal-space projector P_in = S_in · pinv(S)[:N_in,:] (n_meg × n_meg). */
        Eigen::MatrixXd matPinvAll;       /**< Pseudoinverse of [S_in | S_out], shape (N_in+N_out) × n_meg. */
        QVector<int>    megChannelIdx;    /**< Original channel indices in FiffInfo for the MEG rows. */
        int             iOrderIn  = 8;   /**< Internal order used. */
        int             iOrderOut = 3;   /**< External order used. */
        int             iNin      = 0;   /**< N_in  = iOrderIn*(iOrderIn+2). */
        int             iNout     = 0;   /**< N_out = iOrderOut*(iOrderOut+2). */
    };

    //=========================================================================================================
    /**
     * Build the SSS spherical-harmonic basis from MEG sensor geometry.
     *
     * Only channels with kind == FIFFV_MEG_CH or FIFFV_REF_MEG_CH are used.
     * Sensor positions and normals are taken from FiffChInfo::chpos (coil-centre single-point model).
     *
     * @param[in] fiffInfo  Measurement info containing channel positions.
     * @param[in] params    SSS configuration (order, origin, regularisation).
     *
     * @return Basis struct containing all projectors needed for SSS and tSSS.
     */
    static Basis computeBasis(const FIFFLIB::FiffInfo& fiffInfo,
                               const Params&            params = Params());

    //=========================================================================================================
    /**
     * Apply SSS to MEG data — suppress external (environmental) interference.
     *
     * @param[in] matData   Full sensor data (n_channels × n_samples).
     *                      Non-MEG channels are passed through unchanged.
     * @param[in] basis     Precomputed basis from computeBasis().
     *
     * @return SSS-cleaned data (n_channels × n_samples).
     */
    static Eigen::MatrixXd apply(const Eigen::MatrixXd& matData,
                                  const Basis&           basis);

    //=========================================================================================================
    /**
     * Apply temporal SSS (tSSS) — additionally suppress near-field artefacts.
     *
     * tSSS processes the data in sliding windows and removes the temporal subspace of the
     * external expansion that exceeds @p dCorrLimit (relative to the dominant singular value).
     *
     * @param[in] matData       Full sensor data (n_channels × n_samples).
     * @param[in] basis         Precomputed basis from computeBasis().
     * @param[in] iBufferLength Window length in samples (default 10000 ≈ 10 s at 1 kHz).
     * @param[in] dCorrLimit    Correlation threshold [0, 1]; singular vectors of the external
     *                          subspace whose normalised value exceeds this are removed from the
     *                          internal expansion (default 0.98).
     *
     * @return tSSS-cleaned data (n_channels × n_samples).
     */
    static Eigen::MatrixXd applyTemporal(const Eigen::MatrixXd& matData,
                                          const Basis&           basis,
                                          int                    iBufferLength = 10000,
                                          double                 dCorrLimit    = 0.98);

private:
    //=========================================================================================================
    /**
     * Compute 4π-normalised associated Legendre polynomial table and its θ-derivative table
     * for the given cos(θ) and sin(θ).
     *
     * After the call:
     *   P(l, m)  = N_l^m * P_l^m(cos θ)
     *   dP(l, m) = N_l^m * dP_l^m/dθ (derived using the recurrence for the un-normalised P)
     *
     * @param[in]  lmax      Maximum degree.
     * @param[in]  cosTheta  cos(θ).
     * @param[in]  sinTheta  sin(θ) (must be ≥ 0).
     * @param[out] P         (lmax+2) × (lmax+2) normalised ALP table.
     * @param[out] dP        (lmax+2) × (lmax+2) normalised dP/dθ table.
     */
    static void computeNormALP(int             lmax,
                                double          cosTheta,
                                double          sinTheta,
                                Eigen::MatrixXd& P,
                                Eigen::MatrixXd& dP);

    //=========================================================================================================
    /**
     * Compute the Cartesian gradient of a single spherical-harmonic basis function evaluated
     * at position @p rPos.
     *
     * For internal basis (bInternal = true):  grad(r^l  * Y_l^m)
     * For external basis (bInternal = false): grad(r^{-(l+1)} * Y_l^m)
     *
     * @param[in] l          Degree (l ≥ 1).
     * @param[in] m          Order (-l ≤ m ≤ l).
     * @param[in] bInternal  True for internal, false for external expansion.
     * @param[in] rPos       Sensor position relative to SSS origin (metres).
     * @param[in] P          Normalised ALP table from computeNormALP().
     * @param[in] dP         Normalised dP/dθ table from computeNormALP().
     * @param[in] cosTheta   cos(θ) at rPos.
     * @param[in] sinTheta   sin(θ) at rPos.
     * @param[in] cosPhi     cos(φ) at rPos.
     * @param[in] sinPhi     sin(φ) at rPos.
     *
     * @return 3-vector gradient in Cartesian coordinates.
     */
    static Eigen::Vector3d basisGradCart(int                     l,
                                          int                     m,
                                          bool                    bInternal,
                                          const Eigen::Vector3d&  rPos,
                                          const Eigen::MatrixXd&  P,
                                          const Eigen::MatrixXd&  dP,
                                          double                  cosTheta,
                                          double                  sinTheta,
                                          double                  cosPhi,
                                          double                  sinPhi);
};

} // namespace UTILSLIB

#endif // SSS_DSP_H
