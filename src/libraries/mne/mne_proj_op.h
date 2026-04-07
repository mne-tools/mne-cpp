//=============================================================================================================
/**
 * @file     mne_proj_op.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    MNEProjOp class declaration.
 *
 */

#ifndef MNEPROJOP_H
#define MNEPROJOP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_named_matrix.h"
#include "mne_proj_item.h"

#include <fiff/fiff_types.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_dir_node.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QTextStream>

#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
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
 */
class MNESHARED_EXPORT MNEProjOp
{
public:
    typedef QSharedPointer<MNEProjOp> SPtr;              /**< Shared pointer type for MNEProjOp. */
    typedef QSharedPointer<const MNEProjOp> ConstSPtr;   /**< Const shared pointer type for MNEProjOp. */

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
     * Clears @ref proj_data, @ref names, @ref nch, and @ref nvec without
     * touching the underlying projection items.
     */
    void free_proj();

    /**
     * @brief Append all projection items from another operator.
     *
     * Each item's active_file flag is preserved.
     *
     * @param[in] from   Source operator whose items are copied.
     *
     * @return Pointer to this operator.
     */
    MNEProjOp* combine(MNEProjOp* from);

    /**
     * @brief Add a projection item with an explicit active/inactive state.
     *
     * The projection kind (MEG/EEG) is auto-detected from channel names.
     *
     * @param[in] vecs       Named matrix holding the projection vectors.
     * @param[in] kind       Projection kind constant.
     * @param[in] desc       Human-readable description of the projection.
     * @param[in] is_active  Whether the item is active on load.
     */
    void add_item_active(const MNENamedMatrix* vecs, int kind, const  QString& desc, bool is_active);

    /**
     * @brief Add a projection item that is active by default.
     *
     * Convenience wrapper around add_item_active() with @c is_active = TRUE.
     *
     * @param[in] vecs   Named matrix holding the projection vectors.
     * @param[in] kind   Projection kind constant.
     * @param[in] desc   Human-readable description of the projection.
     */
    void add_item(const MNENamedMatrix* vecs, int kind, const QString& desc);

    /**
     * @brief Create a deep copy of this projection operator.
     *
     * Copies all items, their vectors, descriptions, and active states.
     *
     * @return A newly allocated copy. Caller takes ownership.
     */
    MNEProjOp* dup() const;

    /**
     * @brief Create an average EEG reference projector.
     *
     * Builds a uniform-weight vector (\f$1/\sqrt{N_{\text{EEG}}}\f$)
     * across all EEG channels.
     *
     * @param[in] chs   Channel information list.
     * @param[in] nch   Number of channels.
     *
     * @return A new projection operator, or NULL if no EEG channels are found.
     *         Caller takes ownership.
     */
    static MNEProjOp* create_average_eeg_ref(const QList<FIFFLIB::FiffChInfo>& chs, int nch);

    /**
     * Count how many active projection vectors affect a given list of
     * channel names.
     *
     * @param[in] list    List of channel names to test.
     * @param[in] nlist   Number of channel names.
     *
     * @return Number of projection vectors that affect at least one channel
     *         in the list (0 if none).
     */
    int affect(const QStringList& list, int nlist);

    /**
     * Count how many active projection vectors affect the given channels.
     * Convenience wrapper that extracts channel names and delegates to
     * affect().
     *
     * @param[in] chs   Channel information list.
     * @param[in] nch   Number of channels.
     *
     * @return Number of affecting projection vectors (0 if none or nch == 0).
     */
    int affect_chs(const QList<FIFFLIB::FiffChInfo> &chs, int nch);

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
     * @return OK on success, FAIL on dimension mismatch.
     */
    int project_vector(Eigen::Ref<Eigen::VectorXf> vec, bool do_complement);

    /**
     * @brief Read all linear projection items from a FIFF tree node.
     *
     * @param[in] stream   An open FIFF stream.
     * @param[in] start    The tree node to search for projection blocks.
     *
     * @return A populated projection operator (possibly with zero items),
     *         or NULL on error. Caller takes ownership.
     */
    static MNEProjOp* read_from_node(FIFFLIB::FiffStream::SPtr& stream,
                                     const FIFFLIB::FiffDirNode::SPtr& start);

    /**
     * Read a projection operator from a FIFF file by path.
     * Convenience wrapper that opens the file and delegates to read_from_node().
     *
     * @param[in] name   Path to the FIFF file.
     *
     * @return The loaded projection operator, or NULL on error.
     *         Caller takes ownership.
     */
    static MNEProjOp* read(const QString& name);

    /**
     * @brief Load and combine SSP projection operators from files for the selected channels.
     *
     * Reads projection items from each file in @p projnames, and if EEG
     * channels are present adds an average EEG reference projector when none
     * is found. Returns nullptr if the resulting projector does not affect
     * any of the given channels.
     *
     * Refactored: make_projection (dipole_fit_setup.c)
     *
     * @param[in]  projnames  List of FIFF file paths containing projection data.
     * @param[in]  chs        Channel information list.
     * @param[in]  nch        Number of channels.
     * @param[out] result     The combined projection operator (nullptr when
     *                        no projection is needed).
     *
     * @return true on success, false on read error.
     */
    static bool makeProjection(const QList<QString>& projnames,
                               const QList<FIFFLIB::FiffChInfo>& chs,
                               int nch,
                               std::unique_ptr<MNEProjOp>& result);

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
    void report_data(QTextStream &out, const QString &tag, bool list_data, const QStringList &exclude);

    /**
     * Write a one-line-per-item summary of all projection items to a text
     * stream (no vector data). Convenience wrapper around report_data().
     *
     * @param[in, out] out   The text stream to write to.
     * @param[in]      tag   Prefix string printed before each line.
     */
    void report(QTextStream &out, const QString &tag);

    //=========================================================================================================
    /**
     * Assign a channel name list to this projection operator, invalidating
     * any previously compiled projector.
     *
     * @param[in] list    Channel name list.
     * @param[in] nlist   Number of channels.
     *
     * @return OK on success.
     */
    int assign_channels(const QStringList& list, int nlist);

    //=========================================================================================================
    /**
     * Compile the projection operator via SVD, excluding bad channels.
     * Active projection items are picked, expanded to the assigned channel list,
     * bad channels are zeroed, and SVD is performed to produce an orthogonal
     * projector matrix stored in proj_data.
     *
     * @param[in] bad    List of bad channel names.
     *
     * @return OK on success, FAIL on error.
     */
    int make_proj_bad(const QStringList& bad);

    //=========================================================================================================
    /**
     * Compile the projection operator via SVD (no bad channels).
     * Convenience wrapper around make_proj_bad().
     *
     * @return OK on success, FAIL on error.
     */
    int make_proj();

    //=========================================================================================================
    /**
     * Apply the compiled projection operator to a double-precision data vector.
     *
     * @param[in,out] vec             Data vector.
     * @param[in]     do_complement   If true, compute (I - P) * vec.
     *
     * @return OK on success, FAIL on dimension mismatch.
     */
    int project_dvector(Eigen::Ref<Eigen::VectorXd> vec, bool do_complement);

    //=========================================================================================================
    /**
     * Apply this projection operator to a covariance matrix from both sides:
     * C' = (I - P) C (I - P)^T (or P C P^T if do_complement is false).
     *
     * @param[in,out] c   The covariance matrix to project.
     *
     * @return OK on success, FAIL on error.
     */
    int apply_cov(MNECovMatrix* c);

public:
    QList<MNELIB::MNEProjItem> items;  /**< The projection items. */
    int         nitems;                 /**< Number of items. */
    QStringList names;                  /**< Names of the channels in the final compiled projector. */
    int         nch;                    /**< Number of channels in the final projector. */
    int         nvec;                   /**< Number of orthogonalized vectors in the final projector. */
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> proj_data; /**< The compiled projector: orthogonalized projection vectors (nvec x nch). */
};

} // NAMESPACE MNELIB

#endif // MNEPROJOP_H
