//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_deriv_set.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Collection of @ref MNELIB::MNEDeriv objects forming a derivation montage.
 *
 * @ref MNELIB::MNEDerivSet wraps a list of channel derivations together
 * with the FIFF read/write needed to round-trip a @c -deriv.fif file.
 * Used by the raw and evoked paths to materialise the derived channels
 * next to the recorded ones.
 */

#ifndef MNEDERIVSET_H
#define MNEDERIVSET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

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

class MNEDeriv;

//=============================================================================================================
/**
 * @brief Collection of MNEDeriv channel derivation definitions loaded from a file.
 */
class MNESHARED_EXPORT MNEDerivSet
{
public:
    typedef QSharedPointer<MNEDerivSet> SPtr;              /**< Shared pointer type for MNEDerivSet. */
    typedef QSharedPointer<const MNEDerivSet> ConstSPtr;   /**< Const shared pointer type for MNEDerivSet. */

    //=========================================================================================================
    /**
     * Constructs the MNE Derivation Set
     */
    MNEDerivSet();

    //=========================================================================================================
    /**
     * Destructor.
     */
    ~MNEDerivSet();

public:
    QList<MNEDeriv*> derivs;       /**< List of derivation items. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEDERIVSET_H
