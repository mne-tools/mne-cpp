//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_source_estimate.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    InvSourceEstimate value type — central source-space data container produced by every mne_inv inverse solver.
 *
 * @ref mne::InvSourceEstimate replaces the legacy MNE-C
 * @c mneStcDataRec record and carries the full output of an inverse
 * solution: an @c (n_sources × n_times) data matrix plus the vertex
 * indices, time origin and sample step. Beyond the dense grid it also
 * holds the optional focal-dipole list (@ref mne::InvFocalDipole), coupling
 * groups (@ref mne::InvSourceCoupling) and pairwise connectivity layers
 * (@ref mne::InvConnectivity), so a single value can represent the output of
 * MNE / dSPM / sLORETA, RAP-MUSIC, MxNE or DICS without losing
 * algorithm-specific by-products. The class implements STC / W binary
 * I/O (round-trip compatible with MNE-Python and MNE-C), label-based
 * slicing via @c mne::FsLabel and copy / reduce / scale operations
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

#include <base/error.h>

#include <Eigen/Core>


#include <filesystem>
#include <span>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Central source-space container produced by every mne_inv inverse solver.
 * Carries the dense @c (n_sources × n_times) data matrix plus vertex
 * indices, time origin and step, and optional focal dipoles, coupling
 * groups and connectivity layers so one value can represent the output
 * of any algorithm in mne_inv. Replaces @c mneStcDataRec of MNE-C and
 * implements round-trip STC / W binary I/O with mne-python and mne-c.
 *
 * @brief Source-space inverse-solution container with dense grid plus optional focal-dipole, coupling and connectivity layers.
 *
 * @snippet ex_inv_api/main.cpp inv_minimum_norm_usage
 *
 * @snippet ex_inv_api/main.cpp inv_source_estimate_io_usage
 */
class INVSHARED_EXPORT InvSourceEstimate
{
public:
    /** Constructs an empty source estimate. */
    InvSourceEstimate() = default;

    //=========================================================================================================
    /**
     * Constructs a source estimate from its data.
     *
     * @param[in] data       Source amplitudes (sources x time points).
     * @param[in] vertices   Source space vertex numbers of the rows of @p data.
     * @param[in] tmin       Time of the first sample in seconds.
     * @param[in] tstep      Time between samples in seconds.
     */
    InvSourceEstimate(Eigen::MatrixXd data, Eigen::VectorXi vertices, float tmin, float tstep);

    //=========================================================================================================
    /**
     * Reads an .stc file like read(); throws mne::Error if it cannot.
     *
     * @param[in] path   The .stc file.
     */
    explicit InvSourceEstimate(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Reads an .stc file, as mne.read_source_estimate does for one hemisphere.
     *
     * @param[in] path   The .stc file.
     *
     * @return The source estimate, or why the file cannot be read: Io if it cannot be opened, Format if
     *         it ends before the data its header announces.
     */
    [[nodiscard]] static mne::Expected<InvSourceEstimate> read(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Writes the estimate as an .stc file, as MNE-C's mne_write_stc_file and mne.SourceEstimate.save do.
     *
     * @param[in] path   The .stc file.
     *
     * @return Nothing, or why the file cannot be written (Io).
     */
    [[nodiscard]] mne::Expected<void> write(const std::filesystem::path& path) const;

    //=========================================================================================================
    /**
     * Writes a surface estimate as the "<base>-lh.stc" and "<base>-rh.stc" pair mne.read_source_estimate
     * expects, splitting at n_vertices_lh.
     *
     * @param[in] base   Path without the "-lh.stc"/"-rh.stc" suffix; a trailing "-lh.stc", "-rh.stc" or
     *                   ".stc" is dropped.
     *
     * @return Nothing, or why the pair cannot be written: Format if the split point is unknown or the
     *         data do not match the vertices, Io if a file cannot be written.
     */
    [[nodiscard]] mne::Expected<void> write_hemisphere_pair(const std::filesystem::path& base) const;

    //=========================================================================================================
    /**
     * Reads a .w file (one value per vertex), as mne.read_w does.
     *
     * @param[in] path   The .w file.
     *
     * @return A single-column source estimate, or why the file cannot be read (Io or Format).
     */
    [[nodiscard]] static mne::Expected<InvSourceEstimate> read_w(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Writes the first time point as a .w file, as mne.write_w does.
     *
     * @param[in] path   The .w file.
     *
     * @return Nothing, or why the file cannot be written: Format if the estimate is empty, Io otherwise.
     */
    [[nodiscard]] mne::Expected<void> write_w(const std::filesystem::path& path) const;

    //=========================================================================================================
    /**
     * Resets to an empty estimate.
     */
    void clear();

    //=========================================================================================================
    /**
     * @param[in] start   First sample.
     * @param[in] n       Number of samples.
     *
     * @return The estimate restricted to samples [start, start + n), its tmin moved to the first of them.
     */
    InvSourceEstimate reduce(int start, int n) const;

    /** @return Whether the estimate holds no time axis. */
    bool empty() const
    {
        return tstep == -1;
    }

    /** @return The number of time samples. */
    int samples() const
    {
        return static_cast<int>(data.cols());
    }

    //=========================================================================================================
    /**
     * @param[in] labels      The labels.
     * @param[in] clustered   Whether the rows are clustered ROIs (matched by label id) rather than vertices.
     *
     * @return The rows of the sources inside the labels, in row order.
     */
    Eigen::VectorXi indices_by_label(std::span<const mne::FsLabel> labels, bool clustered) const;

    /** @return Whether the estimate holds grid data (distributed methods or RAP-MUSIC amplitudes). */
    bool has_grid_data() const
    {
        return data.size() > 0;
    }

    /** @return Whether the estimate holds correlated source groups (e.g. RAP-MUSIC pairs). */
    bool has_couplings() const
    {
        return !couplings.empty();
    }

    /** @return Whether the estimate holds off-grid focal dipoles. */
    bool has_focal_dipoles() const
    {
        return !focalDipoles.empty();
    }

    /** @return Whether the estimate carries explicit source positions (discrete source space). */
    bool has_positions() const
    {
        return positions.rows() > 0;
    }

    /** @return Whether the estimate holds connectivity results. */
    bool has_connectivity() const
    {
        return !connectivity.empty();
    }

    Eigen::MatrixXd data;     /**< Source amplitudes (sources x time points). */
    Eigen::VectorXi vertices; /**< Vertex of each row; the ROI index for clustered estimates. */
    Eigen::RowVectorXf times; /**< Time of each sample in seconds. */
    float tmin = 0.0f;        /**< Time of the first sample in seconds. */
    float tstep = -1.0f;      /**< Time between samples in seconds; -1 for an empty estimate. */
    int nVerticesLh = -1;     /**< Number of leading rows from the left hemisphere; -1 when unknown, as for non-surface source spaces. */

    InvEstimateMethod method = InvEstimateMethod::Unknown;            /**< The inverse method that produced this estimate. */
    InvSourceSpaceType sourceSpaceType = InvSourceSpaceType::Unknown; /**< Source space type (surface, volume, mixed, discrete). */
    InvOrientationType orientationType = InvOrientationType::Unknown; /**< Orientation constraint used (fixed, free, loose). */

    Eigen::MatrixX3f positions;                /**< Positions (m) in head coordinates, one row per source; empty when they live in a source space. */
    std::vector<InvSourceCoupling> couplings;  /**< Correlated source groups overlaid on the grid. */
    std::vector<InvFocalDipole> focalDipoles;  /**< Off-grid focal dipoles (ECD results). */
    std::vector<InvConnectivity> connectivity; /**< Pairwise connectivity between sources (one per metric / frequency band). */

private:
    /** Recomputes times from tmin, tstep and the number of samples. */
    void update_times();
};

} //NAMESPACE

#endif // INV_SOURCE_ESTIMATE_H
