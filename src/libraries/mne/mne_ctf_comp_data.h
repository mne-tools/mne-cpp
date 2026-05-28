//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_ctf_comp_data.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Single CTF reference-sensor compensation matrix labelled by kind.
 *
 * @ref MNELIB::MNECTFCompData stores one entry of the CTF software
 * compensation chain (1st / 2nd / 3rd gradient) as a labelled matrix
 * operating from reference to primary channels. FIFF tags:
 * @c FIFFB_MNE_CTF_COMP_DATA, @c FIFF_MNE_CTF_COMP_KIND,
 * @c FIFF_MNE_CTF_COMP_CALIBRATED, @c FIFF_MNE_CTF_COMP_DATA.
 */

#ifndef MNECTFCOMPDATA_H
#define MNECTFCOMPDATA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

#include "mne_named_matrix.h"
#include <fiff/fiff_sparse_matrix.h>

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

//=============================================================================================================
/**
 * @brief Represents a single CTF compensation data element.
 *
 * Holds the compensation matrix together with optional sparse
 * pre-/post-selectors and intermediate computation buffers used
 * by MNECTFCompDataSet::apply() and apply_transpose().
 */
class MNESHARED_EXPORT MNECTFCompData
{
public:
    typedef QSharedPointer<MNECTFCompData> SPtr;              /**< Shared pointer type for MNECTFCompData. */
    typedef QSharedPointer<const MNECTFCompData> ConstSPtr;   /**< Const shared pointer type for MNECTFCompData. */

    //=========================================================================================================
    /**
     * Constructs an empty MNE CTF Compensation Data object.
     */
    MNECTFCompData();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] comp  The compensation data to copy.
     */
    MNECTFCompData(const MNECTFCompData& comp);

    //=========================================================================================================
    /**
     * Destructor.
     */
    ~MNECTFCompData();

    /**
     * Apply or remove channel calibration to/from the compensation matrix
     * by scaling each element by (row_cal / col_cal) or its inverse.
     *
     * @param[in] chs    Channel information used for calibration factors.
     * @param[in] nch    Number of channels.
     * @param[in] do_it  If true, apply calibration; if false, remove it.
     *
     * @return OK on success, FAIL if a channel is not found.
     */
    int calibrate(const QList<FIFFLIB::FiffChInfo> &chs,
                  int            nch,
                  bool           do_it);

public:
    int             kind;                   /**< The CTF compensation kind constant. */
    int             mne_kind;               /**< MNE-internal compensation kind. */
    bool            calibrated;             /**< Whether the coefficients are already calibrated. */
    std::unique_ptr<MNENamedMatrix>            data;      /**< The compensation matrix. */
    std::unique_ptr<FIFFLIB::FiffSparseMatrix>   presel;    /**< Sparse selector applied before compensation. */
    std::unique_ptr<FIFFLIB::FiffSparseMatrix>   postsel;   /**< Sparse selector applied after compensation. */
    Eigen::VectorXf  presel_data;            /**< Intermediate buffer for pre-selection results. */
    Eigen::VectorXf  comp_data;              /**< Intermediate buffer for compensation results. */
    Eigen::VectorXf  postsel_data;           /**< Intermediate buffer for post-selection results. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNECTFCOMPDATA_H
