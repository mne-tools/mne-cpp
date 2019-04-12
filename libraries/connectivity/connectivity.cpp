//=============================================================================================================
/**
* @file     connectivity.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Connectivity class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connectivity.h"

#include "connectivitysettings.h"
#include "network/network.h"
#include "metrics/correlation.h"
#include "metrics/crosscorrelation.h"
#include "metrics/coherence.h"
#include "metrics/imagcoherence.h"
#include "metrics/phaselagindex.h"
#include "metrics/phaselockingvalue.h"
#include "metrics/weightedphaselagindex.h"
#include "metrics/unbiasedsquaredphaselagindex.h"
#include "metrics/debiasedsquaredweightedphaselagindex.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QFutureSynchronizer>
#include <QtConcurrent>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CONNECTIVITYLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Connectivity::Connectivity()
{
}


//*************************************************************************************************************

QList<Network> Connectivity::calculate(ConnectivitySettings& connectivitySettings)
{
    QStringList lMethods = connectivitySettings.getConnectivityMethods();
    QList<Network> results;
    QElapsedTimer timer;

    int iNTrials = connectivitySettings.getTrialData().size();

    if(lMethods.contains("WPLI")) {
        timer.restart();
        Network networktemp = WeightedPhaseLagIndex::calculate(connectivitySettings);
        qWarning() << "Connectivity::calculateMultiMethods - Calculated WPLI for" << iNTrials << "trials in"<< timer.elapsed() << "msecs.";
        results.append(networktemp);
    }

    if(lMethods.contains("USPLI")) {
        timer.restart();
        results.append(UnbiasedSquaredPhaseLagIndex::calculate(connectivitySettings));
        qWarning() << "Connectivity::calculateMultiMethods - Calculated USPLI for" << iNTrials << "trials in" << timer.elapsed() << "msecs.";
    }

    if(lMethods.contains("COR")) {
        timer.restart();
        results.append(Correlation::calculate(connectivitySettings));
        qWarning() << "Connectivity::calculateMultiMethods - Calculated COR for" << iNTrials << "trials in" << timer.elapsed() << "msecs.";
    }

    if(lMethods.contains("XCOR")) {
        timer.restart();
        results.append(CrossCorrelation::calculate(connectivitySettings));
        qWarning() << "Connectivity::calculateMultiMethods - Calculated XCOR for" << iNTrials << "trials in" << timer.elapsed() << "msecs.";
    }

    if(lMethods.contains("PLI")) {
        timer.restart();
        results.append(PhaseLagIndex::calculate(connectivitySettings));
        qWarning() << "Connectivity::calculateMultiMethods - Calculated PLI for" << iNTrials << "trials in" << timer.elapsed() << "msecs.";
    }

    if(lMethods.contains("COH")) {
        timer.restart();
        results.append(Coherence::calculate(connectivitySettings));
        qWarning() << "Connectivity::calculateMultiMethods - Calculated COH for" << iNTrials << "trials in" << timer.elapsed() << "msecs.";
    }

    if(lMethods.contains("IMAGCOH")) {
        timer.restart();
        results.append(ImagCoherence::calculate(connectivitySettings));
        qWarning() << "Connectivity::calculateMultiMethods - Calculated IMAGCOH for" << iNTrials << "trials in" << timer.elapsed() << "msecs.";
    }

    if(lMethods.contains("PLV")) {
        timer.restart();
        results.append(PhaseLockingValue::calculate(connectivitySettings));
        qWarning() << "Connectivity::calculateMultiMethods - Calculated PLV for" << iNTrials << "trials in" << timer.elapsed() << "msecs.";
    }

    if(lMethods.contains("DSWPLI")) {
        timer.restart();
        results.append(DebiasedSquaredWeightedPhaseLagIndex::calculate(connectivitySettings));
        qWarning() << "Connectivity::calculateMultiMethods - Calculated DSWPLI for" << iNTrials << "trials in" << timer.elapsed() << "msecs.";
    }

    return results;
}
