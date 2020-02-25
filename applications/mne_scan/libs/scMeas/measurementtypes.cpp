//=============================================================================================================
/**
 * @file     measurementtypes.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     August, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Definition of the MeasurementTypes class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "measurementtypes.h"

#include "measurement.h"
#include "realtimesamplearray.h"
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
    qRegisterMetaType< RealTimeSampleArray::SPtr >("RealTimeSampleArray::SPtr");
    qRegisterMetaType< RealTimeMultiSampleArray::SPtr >("RealTimeMultiSampleArray::SPtr");
    qRegisterMetaType< Numeric::SPtr >("Numeric::SPtr");
    qRegisterMetaType< RealTimeSpectrum::SPtr >("RealTimeSpectrum::SPtr");
    qRegisterMetaType< RealTimeSourceEstimate::SPtr >("RealTimeSourceEstimate::SPtr");
    qRegisterMetaType< RealTimeConnectivityEstimate::SPtr >("RealTimeConnectivityEstimate::SPtr");
    qRegisterMetaType< RealTimeCov::SPtr >("RealTimeCov::SPtr");
    qRegisterMetaType< RealTimeEvokedSet::SPtr >("RealTimeEvokedSet::SPtr");
    qRegisterMetaType< RealTimeSampleArrayChInfo::SPtr >("RealTimeSampleArrayChInfo::SPtr");
}
