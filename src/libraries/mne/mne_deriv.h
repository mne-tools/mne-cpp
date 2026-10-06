//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_deriv.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    One channel derivation: a sparse matrix whose rows define virtual channels as weighted sums of recorded ones.
 *
 * @ref MNELIB::MNEDeriv holds the derivation matrix of one file or montage
 * (rows = derived channels, columns = input channels) together with the
 * state of matching it to a recording. Derivations expose bipolar montages
 * or re-referenced channels without modifying the raw data; @ref MNELIB::MNERawData
 * applies the matched derivation while reading.
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
 *
 * @snippet ex_mne_api/main.cpp mne_deriv_set_usage
 */
class MNESHARED_EXPORT MNEDeriv
{
public:
    typedef QSharedPointer<MNEDeriv> SPtr;            /**< Shared pointer type for MNEDeriv. */
    typedef QSharedPointer<const MNEDeriv> ConstSPtr; /**< Const shared pointer type for MNEDeriv. */

    //=========================================================================================================
    /**
     * Constructs an empty MNE Derivation.
     */
    MNEDeriv();

    //=========================================================================================================
    /**
     * Copies the derivation including its matrix.
     *
     * @param[in] other   The derivation to copy.
     */
    MNEDeriv(const MNEDeriv& other);

    //=========================================================================================================
    /**
     * Destructor.
     */
    ~MNEDeriv();

    //=========================================================================================================
    /**
     * Checks every derived channel against channel info (MNE-C @c mne_validate_deriv).
     *
     * A derived channel is valid if all its inputs are present and of the same kind and
     * unit; valid and chs are filled, chs with the info of the first input.
     *
     * @param[in] chInfo   Channel info of the recording.
     *
     * @return The number of valid derived channels.
     */
    int validate(const QList<FIFFLIB::FiffChInfo>& chInfo);

public:
    QString filename;                                 /**< Source file name the derivation was loaded from. */
    QString shortname;                                /**< Short nickname for this derivation. */
    std::unique_ptr<MNESparseNamedMatrix> deriv_data; /**< The derivation data itself (sparse named matrix). */
    Eigen::VectorXi in_use;                           /**< Per-column count of non-zero elements in the derivation data. */
    Eigen::VectorXi valid;                            /**< Per-derivation validity flags considering input channel units. */
    QList<FIFFLIB::FiffChInfo> chs;                   /**< First matching channel info for each derivation. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEDERIV_H
