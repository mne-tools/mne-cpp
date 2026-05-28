//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_deriv.h
 * @since 2026
 * @date  March 2026
 * @brief Single channel derivation (linear combination of physical channels exposed as a virtual channel).
 *
 * @ref MNELIB::MNEDeriv stores one row of a derivation matrix together
 * with the name of the resulting virtual channel and the names of the
 * physical sources it depends on. Derivations are used by the legacy
 * MNE-C tooling to expose bipolar montages or laplacian channels without
 * modifying the underlying raw data.
 */

#ifndef MNEDERIV_H
#define MNEDERIV_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_sparse_named_matrix.h"

#include <fiff/fiff_types.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>

#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
/**
 * @brief One item in a derivation data set.
 *
 * Holds a sparse named matrix of derivation coefficients together with
 * validity and usage metadata and matched channel information.
 */
class MNESHARED_EXPORT MNEDeriv
{
public:
    typedef QSharedPointer<MNEDeriv> SPtr;              /**< Shared pointer type for MNEDeriv. */
    typedef QSharedPointer<const MNEDeriv> ConstSPtr;   /**< Const shared pointer type for MNEDeriv. */

    //=========================================================================================================
    /**
     * Constructs an empty MNE Derivation.
     */
    MNEDeriv();

    //=========================================================================================================
    /**
     * Destructor.
     */
    ~MNEDeriv();

public:
    QString                  filename;   /**< Source file name the derivation was loaded from. */
    QString                  shortname;  /**< Short nickname for this derivation. */
    std::unique_ptr<MNESparseNamedMatrix> deriv_data; /**< The derivation data itself (sparse named matrix). */
    Eigen::VectorXi          in_use;     /**< Per-column count of non-zero elements in the derivation data. */
    Eigen::VectorXi          valid;      /**< Per-derivation validity flags considering input channel units. */
    QList<FIFFLIB::FiffChInfo> chs;      /**< First matching channel info for each derivation. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEDERIV_H
