//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fwd_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     December 2012
 * @brief    Symbol-visibility macro and build-info entry points for the FWDLIB (MEG/EEG forward-modelling) library.
 *
 * FWDLIB implements the forward problem of bioelectromagnetism: given a
 * current dipole at a known source location, compute the magnetic field
 * sampled by each MEG coil and the electric potential sampled by each
 * EEG electrode. Because Maxwell's equations are evaluated in the
 * quasi-static regime (head-scale frequencies ≪ c/L), the lead-field
 * matrix G that maps dipole moments to sensor readings is purely real,
 * time-invariant for a fixed head and sensor array, and re-usable across
 * every subsequent inverse-solution / source-localisation step.
 *
 * This translation unit only defines the FWDSHARED_EXPORT visibility
 * macro and exposes the build-stamp helpers consumed by the library
 * banner; the actual solver primitives live in the sibling headers.
 */

#ifndef FWD_GLOBAL_H
#define FWD_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QtCore/qglobal.h>
#include <utils/buildinfo.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#  define FWDSHARED_EXPORT
#elif defined(MNE_FWD_LIBRARY)
#  define FWDSHARED_EXPORT Q_DECL_EXPORT    /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define FWDSHARED_EXPORT Q_DECL_IMPORT    /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

//=============================================================================================================
/**
 * @namespace FWDLIB
 * @brief Forward modelling — BEM solver, spherical models, sensor/coil definitions and the lead-field assembly that links current dipoles to MEG/EEG sensor readings.
 */
namespace FWDLIB{

//=============================================================================================================
/**
 * Returns build date and time.
 */
FWDSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 */
FWDSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 */
FWDSHARED_EXPORT const char* buildHashLong();
}

#endif // FWD_GLOBAL_H
