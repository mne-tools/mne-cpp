//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_source_estimate.h
 * @since 2026
 * @date  April 2026
 * @brief InvSourceEstimate value type — central source-space data container produced by every INVLIB inverse solver.
 *
 * @ref INVLIB::InvSourceEstimate replaces the legacy MNE-C
 * @c mneStcDataRec record and carries the full output of an inverse
 * solution: an @c (n_sources × n_times) data matrix plus the vertex
 * indices, time origin and sample step. Beyond the dense grid it also
 * holds the optional focal-dipole list (@ref InvFocalDipole), coupling
 * groups (@ref InvSourceCoupling) and pairwise connectivity layers
 * (@ref InvConnectivity), so a single value can represent the output of
 * MNE / dSPM / sLORETA, RAP-MUSIC, MxNE or DICS without losing
 * algorithm-specific by-products. The class implements STC / W binary
 * I/O (round-trip compatible with MNE-Python and MNE-C), label-based
 * slicing via @c FSLIB::FsLabel and copy / reduce / scale operations
 * needed by the visualisation and analysis pipelines.
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
 * Central source-space container produced by every INVLIB inverse solver.
 * Carries the dense @c (n_sources × n_times) data matrix plus vertex
 * indices, time origin and step, and optional focal dipoles, coupling
 * groups and connectivity layers so one value can represent the output
 * of any algorithm in INVLIB. Replaces @c mneStcDataRec of MNE-C and
 * implements round-trip STC / W binary I/O with mne-python and mne-c.
 *
 * @brief Source-space inverse-solution container with dense grid plus optional focal-dipole, coupling and connectivity layers.
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
     * Read a .w file (single time-point source estimate).
     *
     * The .w format stores vertex indices (3-byte big-endian) and float
     * values (4-byte big-endian) for a single time point.
     *
     * @param[in] path   Path to the .w file.
     *
     * @return The source estimate with single-column data.
     *
     * @since 2.2.0
     */
    static InvSourceEstimate read_w(const QString& path);

    //=========================================================================================================
    /**
     * Write the first time point of this source estimate to a .w file.
     *
     * @param[in] path   Path to the .w file to write.
     *
     * @since 2.2.0
     */
    void write_w(const QString& path) const;

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
    Eigen::VectorXi vertices;       /**< The indices of the dipoles in the different source spaces. In the clustered case, holds the ROI indices. */
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
