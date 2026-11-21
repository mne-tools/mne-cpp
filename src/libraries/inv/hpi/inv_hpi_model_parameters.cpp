//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_hpi_model_parameters.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of @ref mne::InvHpiModelParameters (value semantics, equality, coil-count computation).
 *
 * Implements the parameter-list constructor, the copy / assignment /
 * equality operators and the private @c computeNumberOfCoils /
 * @c checkForLineFreq helpers that keep the cached coil count and
 * basic/advanced model selection consistent with the underlying
 * frequency vector.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_hpi_model_parameters.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InvHpiModelParameters::InvHpiModelParameters(std::vector<int> vecHpiFreqs,
                                             const int iSampleFreq,
                                             const int iLineFreq,
                                             const bool bBasic)
: m_vecHpiFreqs(std::move(vecHpiFreqs))
, m_iSampleFreq(iSampleFreq)
, m_iLineFreq(iLineFreq)
, m_bBasic(bBasic)
{
    computeNumberOfCoils();
    checkForLineFreq();
}

//=============================================================================================================

void InvHpiModelParameters::computeNumberOfCoils()
{
    m_iNHpiCoils = static_cast<int>(m_vecHpiFreqs.size());
}

void InvHpiModelParameters::checkForLineFreq()
{
    if (m_iLineFreq == 0) {
        m_bBasic = true;
    }
}
