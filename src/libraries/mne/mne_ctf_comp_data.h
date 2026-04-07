//=============================================================================================================
/**
 * @file     mne_ctf_comp_data.h
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
 * @brief    MNECTFCompData class declaration.
 *
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
