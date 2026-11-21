//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     mne_deriv.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     January 2017
 * @brief    One channel derivation: a sparse matrix whose rows define virtual channels as weighted sums of recorded ones.
 *
 * @ref mne::MNEDeriv holds the derivation matrix of one file or montage
 * (rows = derived channels, columns = input channels) together with the
 * state of matching it to a recording. Derivations expose bipolar montages
 * or re-referenced channels without modifying the raw data; @ref mne::MNERawData
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
// STL INCLUDES
//=============================================================================================================

#include <string>
#include <vector>
#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
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
    int validate(const std::vector<mne::FiffChInfo>& chInfo);

public:
    std::string filename;                             /**< Source file name the derivation was loaded from. */
    std::string shortname;                            /**< Short nickname for this derivation. */
    std::unique_ptr<MNESparseNamedMatrix> deriv_data; /**< The derivation data itself (sparse named matrix). */
    Eigen::VectorXi in_use;                           /**< Per-column count of non-zero elements in the derivation data. */
    Eigen::VectorXi valid;                            /**< Per-derivation validity flags considering input channel units. */
    std::vector<mne::FiffChInfo> chs;                 /**< First matching channel info for each derivation. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE mne_mne

#endif // MNEDERIV_H
