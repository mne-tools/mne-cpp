//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     mneoperator.cpp
 * @author   Florian Schlembach <fschlembach@web.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Jens Haueisen <jens.haueisen@tu-ilmenau.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     February 2014
 * @brief    Implementation of the MNEOperator pre-processing-operator wrapper.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mneoperator.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::disp;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEOperator::MNEOperator()
: m_OperatorType(UNKNOWN)
, m_sName("unknown")
{
}

//=============================================================================================================

MNEOperator::MNEOperator(const MNEOperator& obj)
{
    m_OperatorType = obj.m_OperatorType;
    m_sName = obj.m_sName;
}

//=============================================================================================================

MNEOperator::MNEOperator(OperatorType type)
: m_OperatorType(type)
, m_sName("unknown")
{
}

//=============================================================================================================

MNEOperator::~MNEOperator()
{
}
