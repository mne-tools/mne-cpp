//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     mneoperator.h
 * @author   Florian Schlembach <fschlembach@web.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Jens Haueisen <jens.haueisen@tu-ilmenau.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     February 2014
 * @brief    Light-weight wrapper describing one MNE pre-processing operator (FIR, SSP, compensation) inside a model.
 *
 * MNEOperator stores the operator type, display name, active flag and
 * the underlying matrix / kernel reference. @ref mne::disp::ChannelInfoModel
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

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE mne::disp
//=============================================================================================================

namespace mne::disp
{

//=============================================================================================================
// mne::disp FORWARD DECLARATIONS
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
    enum OperatorType
    {
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
} // NAMESPACE mne_disp

#endif // MNEOPERATOR_H
