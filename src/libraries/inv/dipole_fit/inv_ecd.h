//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_ecd.h
 * @since March 2026
 * @brief Single equivalent current dipole (ECD) with position, moment and per-fit goodness/χ² metrics.
 *
 * @ref INVLIB::InvEcd represents one ECD record produced by the dipole-
 * fit pipeline at one time point: the head-frame position, the dipole
 * moment, the goodness-of-fit, the χ² value and the degrees of freedom /
 * function-evaluation count used by the optimiser. The class mirrors
 * the @c ecdRec record of MNE-C and is the atomic element collected
 * into the @ref InvEcdSet returned by @ref InvDipoleFit.
 */

#ifndef INV_ECD_H
#define INV_ECD_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inv_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QDebug>

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
/**
 * Implements one Electric Current Dipole (Replaces *ecd,ecdRec struct of MNE-C fit_types.h).
 *
 * @brief Single equivalent current dipole with position, orientation, amplitude, and goodness-of-fit.
 */
class INVSHARED_EXPORT InvEcd
{
public:
    typedef QSharedPointer<InvEcd> SPtr;              /**< Shared pointer type for InvEcd. */
    typedef QSharedPointer<const InvEcd> ConstSPtr;   /**< Const shared pointer type for InvEcd. */

    //=========================================================================================================
    /**
     * Constructs the Electric Current Dipole
     */
    InvEcd();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_ECD      Electric Current Dipole which should be copied.
     */
    InvEcd(const InvEcd& p_ECD);

    //=========================================================================================================
    /**
     * Destroys the Electric Current Dipole description
     */
    ~InvEcd();

    //=========================================================================================================
    /**
     * Prints the InvEcd dipole information to the debug output.
     */
    void print() const;

public:
    bool            valid;  /**< Is this dipole valid. */
    float           time;   /**< Time point. */
    Eigen::Vector3f rd;     /**< Dipole location. */
    Eigen::Vector3f Q;      /**< Dipole moment. */
    float           good;   /**< Goodness of fit. */
    float           khi2;   /**< khi^2 value. */
    int             nfree;  /**< Degrees of freedom for the above. */
    int             neval;  /**< Number of function evaluations required for this fit. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE INVLIB

#endif // INV_ECD_H
