//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     realtimesamplearraychinfo.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Definition of the RealTimeSampleArrayChInfo class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimesamplearraychinfo.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCMEASLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeSampleArrayChInfo::RealTimeSampleArrayChInfo()
: m_qStringChName("")
, m_dMinValue(-80000)
, m_dMaxValue(80000)
, m_iKind(0)
, m_iUnit(FIFF_UNIT_NONE)
, m_iCoilType(FIFFV_COIL_NONE)
{
}

//=============================================================================================================

RealTimeSampleArrayChInfo::~RealTimeSampleArrayChInfo()
{
}
