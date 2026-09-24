//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     measurementtypes.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     August, 2013
 * @brief    Definition of the MeasurementTypes class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "measurementtypes.h"

#include "measurement.h"
#include "realtimemultisamplearray.h"
#include "numeric.h"
#include "realtimesourceestimate.h"
#include "realtimeconnectivityestimate.h"
#include "realtimespectrum.h"
#include "realtimesamplearraychinfo.h"
#include "realtimecov.h"
#include "realtimeevokedset.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCMEASLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MeasurementTypes::MeasurementTypes(QObject *parent)
: QObject(parent)
{
}

//=============================================================================================================

void MeasurementTypes::registerTypes()
{
    qRegisterMetaType< Measurement::SPtr >("Measurement::SPtr");
    qRegisterMetaType< RealTimeMultiSampleArray::SPtr >("RealTimeMultiSampleArray::SPtr");
    qRegisterMetaType< Numeric::SPtr >("Numeric::SPtr");
    qRegisterMetaType< RealTimeSpectrum::SPtr >("RealTimeSpectrum::SPtr");
    qRegisterMetaType< RealTimeSourceEstimate::SPtr >("RealTimeSourceEstimate::SPtr");
    qRegisterMetaType< RealTimeConnectivityEstimate::SPtr >("RealTimeConnectivityEstimate::SPtr");
    qRegisterMetaType< RealTimeCov::SPtr >("RealTimeCov::SPtr");
    qRegisterMetaType< RealTimeEvokedSet::SPtr >("RealTimeEvokedSet::SPtr");
    qRegisterMetaType< RealTimeSampleArrayChInfo::SPtr >("RealTimeSampleArrayChInfo::SPtr");
}
