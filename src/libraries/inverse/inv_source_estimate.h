
//=============================================================================================================
/**
 * @file     inv_source_estimate.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2013
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
 * @brief     InvSourceEstimate class declaration.
 *
 */

#ifndef INV_SOURCE_ESTIMATE_H
#define INV_SOURCE_ESTIMATE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_global.h"
#include "inv_types.h"
#include "inv_focal_dipole.h"
#include "inv_source_coupling.h"
#include "inv_connectivity.h"

#include <fs/fs_label.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QList>
#include <QIODevice>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
// MNELIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Source estimation which holds results of MNE-CPP inverse routines. (Replaces *mneStcData,mneStcDataRec struct of MNE-C mne_types.h).
 *
 * @brief Source estimation
 */
class INVSHARED_EXPORT InvSourceEstimate
{
public:
    typedef QSharedPointer<InvSourceEstimate> SPtr;             /**< Shared pointer type for InvSourceEstimate. */
    typedef QSharedPointer<const InvSourceEstimate> ConstSPtr;  /**< Const shared pointer type for InvSourceEstimate. */

    //=========================================================================================================
    /**
     * Default constructor
     */
    InvSourceEstimate();

    //=========================================================================================================
    /**
     * Constructs a source estimation from given data
     *
     * @param[in] p_sol.
     * @param[in] p_vertices.
     * @param[in] p_tmin.
     * @param[in] p_tstep.
     */
    InvSourceEstimate(const Eigen::MatrixXd &p_sol, const Eigen::VectorXi &p_vertices, float p_tmin, float p_tstep);

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_SourceEstimate    Source estimate data which should be copied.
     */
    InvSourceEstimate(const InvSourceEstimate& p_SourceEstimate);

    //=========================================================================================================
    /**
     * Constructs a source estimation, by reading from a IO device.
     *
     * @param[in] p_IODevice     IO device to read from the source estimation.
     *
     */
    InvSourceEstimate(QIODevice &p_IODevice);

    //=========================================================================================================
    /**
     * Initializes source estimate.
     */
    void clear();

    //=========================================================================================================
    /**
     * Reduces the source estimate to selected samples.
     *
     * @param[in] start  The start index to cut the estimate from.
     * @param[in] n      Number of samples to cut from start index.
     */
    InvSourceEstimate reduce(qint32 start, qint32 n);

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
    static bool read(QIODevice &p_IODevice, InvSourceEstimate& p_stc);

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
     * Returns whether SourceEstimate is empty.
     *
     * @return true if is empty, false otherwise.
     */
    inline bool isEmpty() const;

    //=========================================================================================================
    /**
     * Assignment Operator
     *
     * @param[in] rhs     SourceEstimate which should be assigned.
     *
     * @return the copied source estimate.
     */
    InvSourceEstimate& operator= (const InvSourceEstimate &rhs);

    //=========================================================================================================
    /**
     * Returns the number of samples.
     *
     * @return the number of samples.
     */
    int samples() const;

    //=========================================================================================================
    /**
     * Returns the indices of sources in the data matrix based on their beloning label.
     *
     * @param[in] lPickedLabels      The labels base the selection on.
     * @param[in] bIsClustered       Whether the source space was clustered.
     *
     * @return the indices.
     */
    Eigen::VectorXi getIndicesByLabel(const QList<FSLIB::FsLabel> &lPickedLabels, bool bIsClustered) const;

public:
    Eigen::MatrixXd data;           /**< Matrix of shape [n_dipoles x n_times] which contains the data in source space. */
    Eigen::VectorXi vertices;       /**< The indices of the dipoles in the different source spaces. */ //ToDo define is_clustered_result; in clustered case vertices holds the ROI idcs
    Eigen::RowVectorXf times;       /**< The time vector with n_times steps. */
    float tmin;                     /**< Time starting point. */
    float tstep;                    /**< Time steps within the times vector. */

    // --- Metadata ---
    InvEstimateMethod   method;         /**< The inverse method that produced this estimate. */
    InvSourceSpaceType  sourceSpaceType;/**< Source space type (surface, volume, mixed, discrete). */
    InvOrientationType  orientationType;/**< Orientation constraint used (fixed, free, loose). */

    // --- Positions (for discrete source spaces: sEEG contacts, ECoG electrodes, custom ROIs) ---
    Eigen::MatrixX3f    positions;      /**< 3D positions (m) in head coordinates, one row per source. Empty when positions live in an external source space. */

    // --- Coupling layer (e.g. RAP-MUSIC correlated N-tuples on the grid) ---
    std::vector<InvSourceCoupling> couplings;   /**< Correlated source groups overlaid on the grid. */

    // --- Focal layer (e.g. ECD off-grid dipoles) ---
    std::vector<InvFocalDipole>    focalDipoles; /**< Off-grid focal dipoles (ECD results). */

    // --- Connectivity layer (pairwise source connectivity) ---
    std::vector<InvConnectivity>   connectivity; /**< Pairwise connectivity matrices between sources (one per metric / freq band). */

    //=========================================================================================================
    /**
     * Returns true if the estimate contains grid-based data (distributed methods or RAP-MUSIC amplitudes).
     *
     * @return true if grid data is present.
     */
    inline bool hasGridData() const;

    //=========================================================================================================
    /**
     * Returns true if the estimate contains source coupling annotations (e.g. RAP-MUSIC pairs/N-tuples).
     *
     * @return true if couplings are present.
     */
    inline bool hasCouplings() const;

    //=========================================================================================================
    /**
     * Returns true if the estimate contains focal (off-grid) dipole results.
     *
     * @return true if focal dipoles are present.
     */
    inline bool hasFocalDipoles() const;

    //=========================================================================================================
    /**
     * Returns true if the estimate carries explicit source positions (discrete source space).
     *
     * @return true if positions are present.
     */
    inline bool hasPositions() const;

    //=========================================================================================================
    /**
     * Returns true if the estimate contains connectivity results.
     *
     * @return true if connectivity data is present.
     */
    inline bool hasConnectivity() const;

private:
    //=========================================================================================================
    /**
     * Update the times attribute after changing tmin, tmax, or tstep
     */
    void update_times();
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool InvSourceEstimate::isEmpty() const
{
    return tstep == -1;
}

//=============================================================================================================

inline bool InvSourceEstimate::hasGridData() const
{
    return data.size() > 0;
}

//=============================================================================================================

inline bool InvSourceEstimate::hasCouplings() const
{
    return !couplings.empty();
}

//=============================================================================================================

inline bool InvSourceEstimate::hasFocalDipoles() const
{
    return !focalDipoles.empty();
}

//=============================================================================================================

inline bool InvSourceEstimate::hasPositions() const
{
    return positions.rows() > 0;
}

//=============================================================================================================

inline bool InvSourceEstimate::hasConnectivity() const
{
    return !connectivity.empty();
}
} //NAMESPACE

#endif // INV_SOURCE_ESTIMATE_H
