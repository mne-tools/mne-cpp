//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   mfarisyahya <mfarisyahya@gmail.com>
 *   Daniel Strohmeier <daniel.strohmeier@gmail.com>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file connectivity.cpp
 * @since March 2017
 * @brief Implementation of @ref CONNLIB::Connectivity - the dispatcher that runs the requested functional-connectivity metrics over a @ref CONNLIB::ConnectivitySettings batch.
 *
 * The single entry point @ref CONNLIB::Connectivity::calculate inspects the
 * list of method names on @ref CONNLIB::ConnectivitySettings and routes
 * each request to the corresponding estimator (Coherence, Imaginary
 * Coherence, PLI / wPLI / dwPLI / USPLI, PLV, Pearson and cross
 * correlation, Granger Causality, DTF, PDC). All spectral metrics share
 * the DPSS tapered FFT and CSD accumulator cached on the settings object,
 * so requesting several methods at once costs only one FFT pass.
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
#include "metrics/granger_causality.h"
#include "metrics/directed_transfer_function.h"
#include "metrics/partial_directed_coherence.h"

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

using namespace CONNLIB;

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

    if(lMethods.contains("GC")) {
        results.append(GrangerCausality::calculate(connectivitySettings));
    }

    if(lMethods.contains("DTF")) {
        results.append(DirectedTransferFunction::calculate(connectivitySettings));
    }

    if(lMethods.contains("PDC")) {
        results.append(PartialDirectedCoherence::calculate(connectivitySettings));
    }

    qWarning() << "Total" << timer.elapsed();
    qDebug() << "Connectivity::calculateMultiMethods - Calculated"<< lMethods <<"for" << connectivitySettings.size() << "trials in"<< timer.elapsed() << "msecs.";

    return results;
}
