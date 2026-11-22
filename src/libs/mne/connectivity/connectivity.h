//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     connectivity.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     March 2017
 * @brief    Umbrella include header that exposes the full mne_connectivity public surface in a single include.
 *
 * mne_connectivity computes functional connectivity (coherence, phase-lag and phase-locking measures,
 * correlation, Granger causality, DTF, PDC) and represents the result as a @ref mne::Network.
 */

#ifndef CONNECTIVITY_H
#define CONNECTIVITY_H

//=============================================================================================================
// CONNECTIVITY LIBRARY HEADERS
//=============================================================================================================

#include "connectivity_global.h"
#include "metrics/abstractmetric.h"
#include "metrics/correlation.h"
#include "metrics/crosscorrelation.h"
#include "metrics/coherency.h"
#include "metrics/coherence.h"
#include "metrics/imagcoherence.h"
#include "metrics/unbiasedsquaredphaselagindex.h"
#include "metrics/phaselockingvalue.h"
#include "metrics/weightedphaselagindex.h"
#include "metrics/debiasedsquaredweightedphaselagindex.h"
#include "metrics/phaselagindex.h"
#include "metrics/mvar_model.h"
#include "metrics/granger_causality.h"
#include "metrics/directed_transfer_function.h"
#include "metrics/partial_directed_coherence.h"
#include "network/network.h"
#include "network/networknode.h"
#include "network/networkedge.h"
#include "connectivitysettings.h"
#include "compute_connectivity.h"

#endif // CONNECTIVITY_H
