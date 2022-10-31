//=============================================================================================================
/**
 * @file     connectivity.cpp
 * @author   Daniel Strohmeier <Daniel.Strohmeier@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Faris Yahya <mfarisyahya@gmail.com>
 * @since    0.1.0
 * @date     March, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Daniel Strohmeier, Lorenz Esch, Faris Yahya. All rights reserved.
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

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QFutureSynchronizer>
#include <QtConcurrent>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CONNECTIVITYLIB;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Connectivity::Connectivity()
{
}

//=============================================================================================================

QList<Network> Connectivity::calculate(ConnectivitySettings& connectivitySettings)
{
    QStringList lMethods = connectivitySettings.getConnectivityMethods();
    QList<Network> results;
    QElapsedTimer timer;
    timer.start();

    if(lMethods.contains("WPLI")) {
        results.append(WeightedPhaseLagIndex::calculate(connectivitySettings));
    }

    if(lMethods.contains("USPLI")) {
        results.append(UnbiasedSquaredPhaseLagIndex::calculate(connectivitySettings));
    }

    if(lMethods.contains("COR")) {
        results.append(Correlation::calculate(connectivitySettings));
    }

    if(lMethods.contains("XCOR")) {
        results.append(CrossCorrelation::calculate(connectivitySettings));
    }

    if(lMethods.contains("PLI")) {
        results.append(PhaseLagIndex::calculate(connectivitySettings));
    }

    if(lMethods.contains("COH")) {
        results.append(Coherence::calculate(connectivitySettings));
    }

    if(lMethods.contains("IMAGCOH")) {
        results.append(ImagCoherence::calculate(connectivitySettings));
    }

    if(lMethods.contains("PLV")) {
        results.append(PhaseLockingValue::calculate(connectivitySettings));
    }

    if(lMethods.contains("DSWPLI")) {
        results.append(DebiasedSquaredWeightedPhaseLagIndex::calculate(connectivitySettings));
    }

    qWarning() << "Total" << timer.elapsed();
    qDebug() << "Connectivity::calculateMultiMethods - Calculated"<< lMethods <<"for" << connectivitySettings.size() << "trials in"<< timer.elapsed() << "msecs.";

    return results;
}
