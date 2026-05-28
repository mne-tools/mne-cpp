//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_ctf_comp.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @author   Christof Pieloth <pieloth@labp.htwk-leipzig.de>
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     September 2012
 * @brief    CTF / 4D Neuroimaging software gradient-compensation matrix block (FIFFB_MNE_CTF_COMP_DATA).
 *
 * CTF reference-gradient compensation suppresses environmental
 * interference by subtracting a linear combination of reference-channel
 * signals from each gradiometer. The compensation state is stored as a
 * list of named matrices under @c FIFFB_MNE_CTF_COMP /
 * @c FIFFB_MNE_CTF_COMP_DATA, one matrix per supported compensation
 * grade. @ref FiffCtfComp is the C++ wrapper for one of those matrices: a
 * kind tag selecting the grade, save-as-calibrated flag, and a
 * @ref FiffNamedMatrix whose rows are the gradiometer channel names and
 * columns the reference channels.
 *
 * Combined with the source / destination grade fields tracked in
 * @ref FiffInfo this lets @c FiffRawData::compensate move continuously
 * recorded data between compensation grades on demand, matching the
 * @c mne.io.ctf.RawCTF._comp behaviour in MNE-Python.
 */

#ifndef FIFF_CTF_COMP_H
#define FIFF_CTF_COMP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_types.h"
#include "fiff_named_matrix.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
/**
 * @brief One CTF software-gradient compensation matrix: grade kind, calibration flag and the gradiometer × reference named matrix.
 *
 * A list of @ref FiffCtfComp lives inside @ref FiffInfo and lets raw /
 * evoked data be moved between compensation grades by chaining the
 * appropriate matrices. The @c save_calibrated flag controls whether the
 * matrix was stored already-calibrated (and therefore must not be
 * re-multiplied by the channel cals on apply).
 */
class FIFFSHARED_EXPORT FiffCtfComp {

public:
    using SPtr = QSharedPointer<FiffCtfComp>;            /**< Shared pointer type for FiffCtfComp. */
    using ConstSPtr = QSharedPointer<const FiffCtfComp>; /**< Const shared pointer type for FiffCtfComp. */
    using UPtr = std::unique_ptr<FiffCtfComp>;             /**< Unique pointer type for FiffCtfComp. */
    using ConstUPtr = std::unique_ptr<const FiffCtfComp>;  /**< Const unique pointer type for FiffCtfComp. */

    //=========================================================================================================
    /**
     * Constructs the CTF software compensation data
     */
    FiffCtfComp();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffCtfComp   CTF software compensation data which should be copied.
     */
    FiffCtfComp(const FiffCtfComp &p_FiffCtfComp);

    //=========================================================================================================
    /**
     * Destroys the CTF software compensation data.
     */
    ~FiffCtfComp();

    //=========================================================================================================
    /**
     * Initializes the CTF software compensation data.
     */
    void clear();

public:
    fiff_int_t ctfkind;             /**< CTF kind. */
    fiff_int_t kind;                /**< Fiff kind -> fiff_constants.h. */
    bool save_calibrated;           /**< If data should be saved calibrated. */
    Eigen::MatrixXd rowcals;        /**< Row calibrations. */
    Eigen::MatrixXd colcals;        /**< Column calibrations. */
    FiffNamedMatrix::SDPtr data;    /**< Compensation data. */
};
} // NAMESPACE

#endif // FIFF_CTF_COMP_H
