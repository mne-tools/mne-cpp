//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_hpi_model_parameters.cpp
 * @since March 2026
 * @brief Implementation of @ref INVLIB::InvHpiModelParameters (value semantics, equality, coil-count computation).
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
// QT INCLUDES
//=============================================================================================================
#include <QVector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InvHpiModelParameters::InvHpiModelParameters(const QVector<int> vecHpiFreqs,
                                       const int iSampleFreq,
                                       const int iLineFreq,
                                       const bool bBasic)
    : m_vecHpiFreqs(vecHpiFreqs),
      m_iSampleFreq(iSampleFreq),
      m_iLineFreq(iLineFreq),
      m_bBasic(bBasic)
{
    computeNumberOfCoils();
    checkForLineFreq();
}

//=============================================================================================================

InvHpiModelParameters::InvHpiModelParameters(const InvHpiModelParameters& hpiModelParameter)
    : m_vecHpiFreqs(hpiModelParameter.vecHpiFreqs()),
      m_iNHpiCoils(hpiModelParameter.iNHpiCoils()),
      m_iSampleFreq(hpiModelParameter.iSampleFreq()),
      m_iLineFreq(hpiModelParameter.iLineFreq()),
      m_bBasic(hpiModelParameter.bBasic())
{
}

//=============================================================================================================

void InvHpiModelParameters::computeNumberOfCoils()
{
    m_iNHpiCoils = m_vecHpiFreqs.size();
}

void InvHpiModelParameters::checkForLineFreq()
{
    if(m_iLineFreq == 0) {
        m_bBasic = true;
    }
}

//=============================================================================================================

InvHpiModelParameters InvHpiModelParameters::operator= (const InvHpiModelParameters& other)
{
    if (this != &other) {
        m_vecHpiFreqs = other.vecHpiFreqs();
        m_iNHpiCoils = other.iNHpiCoils();
        m_iSampleFreq = other.iSampleFreq();
        m_iLineFreq = other.iLineFreq();
        m_bBasic = other.bBasic();
    }
    return *this;
}
