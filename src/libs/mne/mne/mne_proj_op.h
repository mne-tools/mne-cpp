//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     mne_proj_op.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     January 2017
 * @brief    Composite SSP projection operator P = I - U U^T assembled from a list of @ref mne::MNEProjItem.
 *
 * @ref mne::MNEProjOp is the operator form of the SSP machinery: it
 * stacks the active projection vectors into an orthonormal basis and
 * applies the corresponding rank-one removal to sensor data, leadfields
 * or noise covariances. Used by the raw, evoked and forward paths to
 * keep all data products in the same projected subspace.
 */

#ifndef MNEPROJOP_H
#define MNEPROJOP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_named_matrix.h"
#include "mne_proj_item.h"
#include <mne/base/error.h>
#include <mne/fiff/fiff_types.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_dir_node.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <ostream>
#include <span>
#include <string>
#include <vector>
#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

class MNECovMatrix;

//=============================================================================================================
/**
 * @brief Projection operator managing a set of linear projection items
 *        and the final compiled projector matrix.
 *
 * MNEProjOp aggregates zero or more MNEProjItem entries, each holding a
 * named matrix of projection vectors. When the operator is compiled
 * (make_projector), the individual items are orthogonalised into a single
 * dense projector stored in @ref proj_data.
 *
 * @snippet ex_mne_api/main.cpp mne_raw_data_usage
 */
class MNESHARED_EXPORT MNEProjOp
{
public:
    //=========================================================================================================
    /**
     * @brief Default constructor.
     *
     * Creates an empty operator with no projection items.
     */
    MNEProjOp();

    //=========================================================================================================
    /**
     * @brief Destructor.
     */
    ~MNEProjOp();

    //=========================================================================================================
    /**
     * @brief Release the compiled projector data.
     *
     * Clears @ref proj_data, @ref names and @ref nvec without
     * touching the underlying projection items.
     */
    void free_proj();

    /**
     * @brief Append all projection items from another operator.
     *
     * Each item's active_file flag is preserved.
     *
     * @param[in] from   Source operator whose items are copied.
     */
    void combine(const MNEProjOp& from);

    /**
     * @brief Add a projection item.
     *
     * The projection kind (MEG/EEG) is auto-detected from channel names.
     *
     * @param[in] vecs       Named matrix holding the projection vectors.
     * @param[in] kind       Projection kind constant.
     * @param[in] desc       Human-readable description of the projection.
     * @param[in] is_active  Whether the item is active.
     */
    void add_item(const MNENamedMatrix& vecs, int kind, const std::string& desc, bool is_active = true);

    /**
     * @brief Create a deep copy of this projection operator.
     *
     * Copies all items, their vectors, descriptions, and active states.
     *
     * @return A newly allocated copy. Caller takes ownership.
     */
    std::unique_ptr<MNEProjOp> dup() const;

    /**
     * @brief Create an average EEG reference projector.
     *
     * Builds a uniform-weight vector (\f$1/\sqrt{N_{\text{EEG}}}\f$)
     * across all EEG channels.
     *
     * @param[in] chs   Channels to search for EEG channels.
     *
     * @return A new projection operator, or nullptr if no EEG channels are found.
     */
    static std::unique_ptr<MNEProjOp> create_average_eeg_ref(std::span<const mne::FiffChInfo> chs);

    /**
     * Count how many active projection vectors affect a given list of
     * channel names.
     *
     * @param[in] list    Channel names to test.
     *
     * @return Number of projection vectors that affect at least one channel
     *         in the list (0 if none).
     */
    int affect(std::span<const std::string> list) const;

    /**
     * Count how many active projection vectors affect the given channels.
     *
     * @param[in] chs   Channels to test.
     *
     * @return Number of affecting projection vectors (0 if none).
     */
    int affect_chs(std::span<const mne::FiffChInfo> chs) const;

    /**
     * Apply the compiled projection operator to a data vector in-place.
     *
     * If @p do_complement is true, the projected components are subtracted
     * from @p vec (signal cleaning). Otherwise, @p vec is replaced by the
     * projection itself.
     *
     * @param[in, out] vec            Data vector of length nch.
     * @param[in]      do_complement If true, compute the complement (I - P) * vec.
     *
     * @return false if @p vec does not have one value per assigned channel.
     */
    [[nodiscard]] bool project_vector(Eigen::Ref<Eigen::VectorXf> vec, bool do_complement) const;

    /**
     * @brief Read all linear projection items from a FIFF tree node.
     *
     * @param[in] stream   An open FIFF stream.
     * @param[in] start    The tree node to search for projection blocks.
     *
     * @return A populated projection operator (possibly with zero items),
     *         or NULL on error. Caller takes ownership.
     */
    static std::unique_ptr<MNEProjOp> read_from_node(mne::FiffStream& stream,
                                                     const std::shared_ptr<mne::FiffDirNode>& start);

    /**
     * Read a projection operator from a FIFF file by path.
     * Convenience wrapper that opens the file and delegates to read_from_node().
     *
     * @param[in] name   Path to the FIFF file.
     *
     * @return The loaded projection operator, or NULL on error.
     *         Caller takes ownership.
     */
    static std::unique_ptr<MNEProjOp> read(const std::filesystem::path& name);

    /**
     * @brief Load and combine SSP projection operators from files for the selected channels.
     *
     * Reads projection items from each file in @p projnames, and if EEG
     * channels are present adds an average EEG reference projector when none
     * is found.
     *
     * Refactored: make_projection (dipole_fit_setup.c)
     *
     * @param[in] projnames  FIFF files containing projection data.
     * @param[in] chs        The selected channels.
     *
     * @return The combined projection operator, nullptr when it would not affect
     *         any of @p chs, or the error of a file that cannot be read.
     */
    [[nodiscard]] static mne::Expected<std::unique_ptr<MNEProjOp>> make_projection(const std::vector<std::filesystem::path>& projnames,
                                                                                   std::span<const mne::FiffChInfo> chs);

    /**
     * Write a formatted summary of all projection items to a text stream,
     * optionally including the full projection vector data while zeroing
     * out excluded channels.
     *
     * @param[in, out] out        The text stream to write to.
     * @param[in]      tag        Prefix string printed before each line.
     * @param[in]      list_data  If true, print full vector data.
     * @param[in]      exclude    Channel names to exclude from the display.
     */
    void report_data(std::ostream& out, const std::string& tag, bool list_data, const StringList& exclude) const;

    /**
     * Write a one-line-per-item summary of all projection items to a text
     * stream (no vector data). Convenience wrapper around report_data().
     *
     * @param[in, out] out   The text stream to write to.
     * @param[in]      tag   Prefix string printed before each line.
     */
    void report(std::ostream& out, const std::string& tag) const;

    //=========================================================================================================
    /**
     * Assign a channel name list to this projection operator, invalidating
     * any previously compiled projector.
     *
     * @param[in] list    Channel names, in data order.
     */
    void assign_channels(std::span<const std::string> list);

    //=========================================================================================================
    /**
     * Compile the projection operator via SVD, excluding bad channels.
     * Active projection items are picked, expanded to the assigned channel list,
     * bad channels are zeroed, and SVD is performed to produce an orthogonal
     * projector matrix stored in proj_data.
     *
     * @param[in] bad    Bad channel names.
     *
     * @return false if a projection vector cannot be expanded to the assigned channels.
     */
    [[nodiscard]] bool make_proj(const StringList& bad = {});

    //=========================================================================================================
    /**
     * Apply the compiled projection operator to a double-precision data vector.
     *
     * @param[in,out] vec             Data vector.
     * @param[in]     do_complement   If true, compute (I - P) * vec.
     *
     * @return false if @p vec does not have one value per assigned channel.
     */
    [[nodiscard]] bool project_dvector(Eigen::Ref<Eigen::VectorXd> vec, bool do_complement) const;

    //=========================================================================================================
    /**
     * Apply this projection operator to a covariance matrix from both sides:
     * C' = (I - P) C (I - P)^T (or P C P^T if do_complement is false).
     *
     * @param[in,out] c   The covariance matrix to project.
     *
     * @return false if @p c is not over the assigned channels.
     */
    [[nodiscard]] bool apply_cov(MNECovMatrix& c) const;

public:
    std::vector<mne::MNEProjItem> items;                                             /**< The projection items. */
    StringList names;                                                                /**< Names of the channels in the final compiled projector. */
    int nvec;                                                                        /**< Number of orthogonalized vectors in the final projector. */
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> proj_data; /**< The compiled projector: orthogonalized projection vectors (nvec x names.size()). */
};

} // NAMESPACE mne_mne

#endif // MNEPROJOP_H
