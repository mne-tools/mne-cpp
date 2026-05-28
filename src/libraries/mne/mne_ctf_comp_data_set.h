//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_ctf_comp_data_set.h
 * @since 2026
 * @date  April 2026
 * @brief Set of CTF compensation matrices plus the currently active grade.
 *
 * @ref MNELIB::MNECTFCompDataSet aggregates the @ref MNECTFCompData
 * entries shipped with a CTF dataset and produces, given a target
 * compensation grade, the channel-aligned compensator that must be
 * applied to raw or forward data. Mirrors @c make_compensator in the
 * MNE-Python and MNE-C codebases.
 */

#ifndef MNECTFCOMPDATASET_H
#define MNECTFCOMPDATASET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

#include "mne_named_matrix.h"
#include <fiff/fiff_sparse_matrix.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
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

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class MNECTFCompData;

//=============================================================================================================
/**
 * @brief Collection of CTF third-order gradient compensation operators.
 *
 * Stores all available compensation data sets read from a FIFF file together
 * with the compiled current/undo operator pair used to switch between
 * compensation grades at runtime.
 */
class MNESHARED_EXPORT MNECTFCompDataSet
{
public:
    typedef QSharedPointer<MNECTFCompDataSet> SPtr;             /**< Shared pointer type for MNECTFCompDataSet. */
    typedef QSharedPointer<const MNECTFCompDataSet> ConstSPtr;  /**< Const shared pointer type for MNECTFCompDataSet. */
    typedef std::unique_ptr<MNECTFCompDataSet> UPtr;            /**< Unique pointer type for MNECTFCompDataSet. */

    //=========================================================================================================
    /**
     * Construct an empty compensation data set.
     */
    MNECTFCompDataSet();

    //=========================================================================================================
    /**
     * Copy constructor. Deep-copies all compensation data and the current operator.
     *
     * @param[in] set   The compensation data set to copy.
     */
    MNECTFCompDataSet(const MNECTFCompDataSet& set);

    //=========================================================================================================
    /**
     * Destructor.
     */
    ~MNECTFCompDataSet();

    //=========================================================================================================
    /**
     * Read all CTF compensation data sets (matrices and channel info) from
     * a FIFF file, calibrate them, and return a populated set.
     *
     * @param[in] name   Path to the FIFF file.
     *
     * @return A new compensation data set, or nullptr on failure.
     */
    static std::unique_ptr<MNECTFCompDataSet> read(const QString& name);

    //=========================================================================================================
    /**
     * Build the current compensation operator for the given channel set
     * by locating the matching compensation matrix and constructing
     * pre/post-selection sparse matrices.
     *
     * @param[in] chs        Channels to compensate (may include non-MEG channels).
     * @param[in] nch        Number of channels.
     * @param[in] compchs    Compensation input channels (may include non-reference channels).
     * @param[in] ncomp      Number of compensation input channels.
     *
     * @return OK on success, FAIL on error.
     */
    int make_comp(const QList<FIFFLIB::FiffChInfo>& chs,
                  int nch,
                  QList<FIFFLIB::FiffChInfo> compchs,
                  int ncomp);

    //=========================================================================================================
    /**
     * Write the compensation grade into the upper 16 bits of coil_type
     * for all MEG channels in the list.
     *
     * @param[in, out] chs    Channel information list.
     * @param[in]      nch    Number of channels.
     * @param[in]      comp   Compensation grade to set.
     *
     * @return Number of channels modified.
     */
    static int set_comp(QList<FIFFLIB::FiffChInfo> &chs,
                        int        nch,
                        int        comp);

    //=========================================================================================================
    /**
     * Apply or revert CTF compensation on a single-sample data vector
     * using the current compensation operator.
     *
     * @param[in]      do_it      If true, subtract compensated component; if false, add it back.
     * @param[in, out] data       Data vector to process.
     * @param[in]      compdata   Compensation channel data (may equal data when omitted).
     *
     * @return OK on success, FAIL on error.
     */
    int apply(bool                 do_it,
              Eigen::Ref<Eigen::VectorXf>     data,
              Eigen::Ref<const Eigen::VectorXf> compdata);

    //=========================================================================================================
    /**
     * Overload: apply compensation using the data vector itself as compensation input.
     *
     * @param[in]      do_it   If true, subtract compensated component; if false, add it back.
     * @param[in, out] data    Data vector to process (also used as compensation input).
     *
     * @return OK on success, FAIL on error.
     */
    int apply(bool                 do_it,
              Eigen::Ref<Eigen::VectorXf>     data);

    //=========================================================================================================
    /**
     * Apply or revert CTF compensation across multiple time samples
     * (channels x samples matrix), the transposed equivalent of apply().
     *
     * @param[in]      do_it   If true, apply compensation; if false, revert.
     * @param[in, out] data    Channel-by-sample data matrix (rows = channels, cols = samples).
     *
     * @return OK on success, FAIL on error.
     */
    int apply_transpose(bool              do_it,
                        Eigen::MatrixXf&  data);

    //=========================================================================================================
    /**
     * Extract the compensation grade from MEG channel coil types.
     *
     * @param[in] chs   Channel information list.
     * @param[in] nch   Number of channels.
     *
     * @return The uniform compensation grade, or FAIL if channels have
     *         inconsistent compensation.
     */
    static int get_comp(const QList<FIFFLIB::FiffChInfo>& chs,int nch);

    //=========================================================================================================
    /**
     * Map a gradient compensation integer code to the corresponding CTF
     * compensation constant.
     *
     * @param[in] grad   Simple integer order (0, 1, 2, 3, ...).
     *
     * @return The mapped CTF compensation constant, or the input if no mapping exists.
     */
    static int map_comp_kind(int grad);

    //=========================================================================================================
    /**
     * Return a human-readable description string for a compensation kind constant.
     *
     * @param[in] kind   Compensation kind constant.
     *
     * @return A string describing the compensation (e.g. "third order gradiometer").
     */
    static QString explain_comp(int kind);

    //=========================================================================================================
    /**
     * Configure the full compensation pipeline: determine current compensation,
     * build undo and target operators, and update channel coil types accordingly.
     *
     * @param[in]      compensate_to  Desired compensation grade.
     * @param[in, out] chs            Channels to compensate (coil_type is updated).
     * @param[in]      nchan          Number of channels.
     * @param[in]      comp_chs       Compensation input channels.
     * @param[in]      ncomp_chan     Number of compensation input channels.
     *
     * @return OK on success, FAIL on error.
     */
    int set_compensation(int compensate_to,
                         QList<FIFFLIB::FiffChInfo>& chs,
                         int nchan,
                         QList<FIFFLIB::FiffChInfo> comp_chs,
                         int ncomp_chan);

public:
    std::vector<std::unique_ptr<MNECTFCompData>> comps;   /**< All available compensation data sets. */
    int            ncomp;           /**< Number of compensation data sets. */
    QList<FIFFLIB::FiffChInfo>     chs;    /**< Channel information associated with compensation. */
    int            nch;             /**< Number of channels. */
    std::unique_ptr<MNECTFCompData> undo;           /**< Compensation data to undo the current state. */
    std::unique_ptr<MNECTFCompData> current;        /**< Compiled compensation operator for the current target grade. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNECTFCOMPDATASET_H
