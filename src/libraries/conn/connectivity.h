//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file connectivity.h
 * @since March 2017
 * @brief Front-end dispatcher that runs the requested connectivity metrics over a @ref ConnectivitySettings batch.
 *
 * @ref Connectivity is the single entry point used by GUI plugins (rtfwd,
 * connectivity-estimator) and the @c mne_dipole_fit / batch tools to compute
 * one or more functional-connectivity estimates from the same set of trials.
 * The caller fills a @ref ConnectivitySettings object with the trial data,
 * sampling frequency, FFT length, taper window, source/sensor node
 * positions, and the list of method names ("COH", "IMAGCOH", "PLI",
 * "WPLI", "DSWPLI", "USPLI", "PLV", "COR", "XCOR", "GC", "DTF",
 * "PDC"), and @ref Connectivity::calculate dispatches to the corresponding
 * metric implementations and returns one @ref Network per method.
 *
 * Trial preprocessing (DPSS tapering, FFT, cross- and auto-spectral sums) is
 * cached in the shared @ref ConnectivitySettings::IntermediateSumData so
 * that running several metrics over the same data set does not recompute
 * the FFTs - the dispatcher therefore amortises the spectral cost across
 * all selected estimators in a single pass.
 */

#ifndef CONNECTIVITY_H
#define CONNECTIVITY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "conn_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE CONNLIB
//=============================================================================================================

namespace CONNLIB {

//=============================================================================================================
// CONNLIB FORWARD DECLARATIONS
//=============================================================================================================

class ConnectivitySettings;
class Network;

//=============================================================================================================
/**
 * Dispatcher that runs one or more functional-connectivity estimators over a
 * batch of pre-processed trials and returns one @ref Network per estimator.
 *
 * The selected methods are read from @ref ConnectivitySettings::getConnectivityMethods
 * and matched by string against the supported metric set
 * ("COR", "XCOR", "COH", "IMAGCOH", "PLI", "USPLI", "WPLI", "DSWPLI",
 * "PLV", "GC", "DTF", "PDC"). All spectral metrics share the same DPSS
 * tapered FFT and CSD accumulator inside @ref ConnectivitySettings, so
 * running e.g. PLI + wPLI + dwPLI in one call costs only one FFT pass.
 *
 * @brief Runs the selected functional-connectivity metrics over a @ref ConnectivitySettings batch.
 */
class CONNSHARED_EXPORT Connectivity
{

public:
    typedef QSharedPointer<Connectivity> SPtr;            /**< Shared pointer type for Connectivity. */
    typedef QSharedPointer<const Connectivity> ConstSPtr; /**< Const shared pointer type for Connectivity. */

    //=========================================================================================================
    /**
     * Constructs a Connectivity object.
     */
    explicit Connectivity();

    //=========================================================================================================
    /**
     * Computes the network based on the current settings.
     *
     * @return Returns the list with calculated networks for each provided method.
     */
    static QList<Network> calculate(ConnectivitySettings& connectivitySettings);

protected:
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace CONNLIB

#endif // CONNECTIVITY_H
