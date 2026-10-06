//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_beamformer.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    InvBeamformer value type — container for pre-computed LCMV / DICS spatial filters and associated metadata.
 *
 * @ref INVLIB::InvBeamformer stores the spatial filter weights produced
 * by @ref INVLIB::InvLCMV::makeLCMV "InvLCMV::makeLCMV" or @ref INVLIB::InvDICS::makeDICS "InvDICS::makeDICS" together with the
 * whitener, SSP projector, source-space vertex list, source normals,
 * weight-normalisation mode (none / unit-noise-gain / NAI), max-power
 * orientations and (for DICS) the per-frequency filter stack. Holding
 * all this on one value lets the @c apply* members re-project new data
 * or covariances through a fixed set of filters without rebuilding any
 * of the heavy quantities, mirroring MNE-Python's @c Beamformer dict.
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
 *
 * @snippet ex_inv_beamformer/main.cpp inv_lcmv_usage
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
     *
     * @return True if filter weights are present, false for a default-constructed beamformer.
     */
    inline bool isValid() const;

    //=========================================================================================================
    /**
     * Returns the number of source points.
     *
     * @return Number of source points covered by the filter weights.
     */
    inline int nSources() const;

    //=========================================================================================================
    /**
     * Returns the number of channels.
     *
     * @return Number of sensor channels the filter weights apply to.
     */
    inline int nChannels() const;

    //=========================================================================================================
    /**
     * Returns the number of orientations per source (1 or 3).
     *
     * @return 1 for fixed or max-power orientation, 3 for free orientation.
     */
    inline int nOrient() const;

    //=========================================================================================================
    /**
     * Returns the number of frequency bins (1 for LCMV, >= 1 for DICS).
     *
     * @return Number of frequency bins with separate filter weights.
     */
    inline int nFreqs() const;

public:
    QString kind; /**< "LCMV" or "DICS". */

    // --- Spatial filter weights ---
    std::vector<Eigen::MatrixXd> weights; /**< Filter weights, size 1 for LCMV, n_freqs for DICS.
                                                Each matrix: (n_sources * n_orient, n_channels). */

    // --- Whitening & projection ---
    Eigen::MatrixXd whitener; /**< Whitening matrix (n_channels, n_channels). */
    Eigen::MatrixXd proj;     /**< SSP projection matrix (n_channels, n_channels). */

    // --- Source space info ---
    Eigen::VectorXi vertices;  /**< Source vertex indices. */
    int nVerticesLh = -1;      /**< Leading entries of vertices belonging to the left hemisphere. -1 when unknown, as for a volume source space. */
    Eigen::MatrixX3f sourceNn; /**< Source normals (n_sources, 3). */
    bool isFreOri;             /**< True if free orientation (n_orient = 3). */
    int nSourcesTotal;         /**< Total number of source points. */
    QString srcType;           /**< "surface", "volume", "mixed", "discrete". */
    QString subject;           /**< Subject identifier. */

    // --- Filter parameters ---
    QStringList chNames;             /**< Channel names used for filter computation. */
    BeamformerWeightNorm weightNorm; /**< Applied weight normalization. */
    BeamformerPickOri pickOri;       /**< Orientation picking mode. */
    BeamformerInversion inversion;   /**< Inversion method used. */
    double reg;                      /**< Regularization parameter used. */
    int rank;                        /**< Data covariance rank used. */

    // --- Max-power orientation (if pickOri == MaxPower) ---
    Eigen::MatrixX3d maxPowerOri; /**< Optimal orientation per source (n_sources, 3). Empty if not max-power. */

    // --- Frequency info (DICS only) ---
    Eigen::VectorXd frequencies; /**< Center frequencies for DICS (Hz). Empty for LCMV. */
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
