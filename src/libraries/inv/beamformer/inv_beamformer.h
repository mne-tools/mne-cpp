//=============================================================================================================
/**
 * @file     inv_beamformer.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    2.1.0
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
 * @brief    InvBeamformer class declaration — stores computed beamformer spatial filters.
 *
 */

#ifndef INV_BEAMFORMER_H
#define INV_BEAMFORMER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inv_global.h"
#include "inv_beamformer_settings.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QList>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
/**
 * Stores pre-computed beamformer spatial filter weights and associated metadata.
 *
 * For LCMV: weights is a single matrix (n_sources * n_orient, n_channels).
 * For DICS: weights is a vector of matrices, one per frequency bin.
 *
 * This class is the C++ equivalent of MNE-Python's Beamformer dict.
 *
 * @brief Computed beamformer spatial filter container.
 */
class INVSHARED_EXPORT InvBeamformer
{
public:
    typedef QSharedPointer<InvBeamformer> SPtr;
    typedef QSharedPointer<const InvBeamformer> ConstSPtr;

    //=========================================================================================================
    /**
     * Default constructor — produces an empty (invalid) beamformer.
     */
    InvBeamformer();

    //=========================================================================================================
    /**
     * Returns true if this beamformer contains valid filter weights.
     */
    inline bool isValid() const;

    //=========================================================================================================
    /**
     * Returns the number of source points.
     */
    inline int nSources() const;

    //=========================================================================================================
    /**
     * Returns the number of channels.
     */
    inline int nChannels() const;

    //=========================================================================================================
    /**
     * Returns the number of orientations per source (1 or 3).
     */
    inline int nOrient() const;

    //=========================================================================================================
    /**
     * Returns the number of frequency bins (1 for LCMV, >= 1 for DICS).
     */
    inline int nFreqs() const;

public:
    QString kind;                       /**< "LCMV" or "DICS". */

    // --- Spatial filter weights ---
    std::vector<Eigen::MatrixXd> weights;  /**< Filter weights, size 1 for LCMV, n_freqs for DICS.
                                                Each matrix: (n_sources * n_orient, n_channels). */

    // --- Whitening & projection ---
    Eigen::MatrixXd whitener;           /**< Whitening matrix (n_channels, n_channels). */
    Eigen::MatrixXd proj;               /**< SSP projection matrix (n_channels, n_channels). */

    // --- Source space info ---
    Eigen::VectorXi vertices;           /**< Source vertex indices. */
    Eigen::MatrixX3f sourceNn;          /**< Source normals (n_sources, 3). */
    bool isFreOri;                      /**< True if free orientation (n_orient = 3). */
    int nSourcesTotal;                  /**< Total number of source points. */
    QString srcType;                    /**< "surface", "volume", "mixed", "discrete". */
    QString subject;                    /**< Subject identifier. */

    // --- Filter parameters ---
    QStringList chNames;                /**< Channel names used for filter computation. */
    BeamformerWeightNorm weightNorm;    /**< Applied weight normalization. */
    BeamformerPickOri pickOri;          /**< Orientation picking mode. */
    BeamformerInversion inversion;      /**< Inversion method used. */
    double reg;                         /**< Regularization parameter used. */
    int rank;                           /**< Data covariance rank used. */

    // --- Max-power orientation (if pickOri == MaxPower) ---
    Eigen::MatrixX3d maxPowerOri;       /**< Optimal orientation per source (n_sources, 3). Empty if not max-power. */

    // --- Frequency info (DICS only) ---
    Eigen::VectorXd frequencies;        /**< Center frequencies for DICS (Hz). Empty for LCMV. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool InvBeamformer::isValid() const
{
    return !weights.empty() && weights[0].size() > 0;
}

//=============================================================================================================

inline int InvBeamformer::nSources() const
{
    return nSourcesTotal;
}

//=============================================================================================================

inline int InvBeamformer::nChannels() const
{
    return weights.empty() ? 0 : static_cast<int>(weights[0].cols());
}

//=============================================================================================================

inline int InvBeamformer::nOrient() const
{
    return isFreOri ? 3 : 1;
}

//=============================================================================================================

inline int InvBeamformer::nFreqs() const
{
    return static_cast<int>(weights.size());
}

} // NAMESPACE INVLIB

#endif // INV_BEAMFORMER_H
