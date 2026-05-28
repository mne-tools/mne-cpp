//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mneoperator.h
 * @since July 2018
 * @brief Light-weight wrapper describing one MNE pre-processing operator (FIR, SSP, compensation) inside a model.
 *
 * MNEOperator stores the operator type, display name, active flag and
 * the underlying matrix / kernel reference. @ref ChannelInfoModel
 * uses these wrappers to track which operators a channel is
 * currently subject to so the delegate can render the
 * post-processing version of the signal.
 */

#ifndef MNEOPERATOR_H
#define MNEOPERATOR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * @brief Light-weight wrapper describing one MNE pre-processing operator (FIR / SSP / compensation).
 *
 * Stores the operator type, display name, active flag and the
 * underlying matrix / kernel reference so @ref ChannelInfoModel can
 * track which operators a channel is currently subject to.
 */
class DISPSHARED_EXPORT MNEOperator
{

public:
    typedef QSharedPointer<MNEOperator> SPtr;              /**< Shared pointer type for MNEOperator. */
    typedef QSharedPointer<const MNEOperator> ConstSPtr;   /**< Const shared pointer type for MNEOperator. */

    enum OperatorType {
        FILTER,
        PCA,
        AVERAGE,
        UNKNOWN
    } m_OperatorType;

    MNEOperator();

    MNEOperator(const MNEOperator& obj);

    MNEOperator(OperatorType type);

    //=========================================================================================================
    /**
     * Destructor
     */
    virtual ~MNEOperator();

    QString m_sName;
};
} // NAMESPACE DISPLIB

#endif // MNEOPERATOR_H
