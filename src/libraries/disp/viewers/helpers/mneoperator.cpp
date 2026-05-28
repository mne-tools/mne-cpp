//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mneoperator.cpp
 * @since July 2018
 * @brief Implementation of the MNEOperator pre-processing-operator wrapper.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mneoperator.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

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
