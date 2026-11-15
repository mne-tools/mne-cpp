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
 * @brief    Symbol-visibility macro and build-info entry points for the mne_fwd (MEG/EEG forward-modelling) library.
 *
 * mne_fwd implements the forward problem of bioelectromagnetism: given a
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

#include <base/buildinfo.h>
#include <base/string_utils.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define FWDSHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_FWD_LIBRARY)
#define FWDSHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define FWDSHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define FWDSHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
#endif

// Exported classes hold Eigen and standard-library members and share their ABI with clients built by the same
// compiler, so MSVC's warnings about members and bases without dll-interface do not apply (Qt disables them alike).
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

namespace mne
{

using mne::StringList;

//=============================================================================================================
/**
 * Returns build date and time.
 *
 * @return Static C string with the compile date and time, e.g. "Sep 30 2026 14:05:12".
 */
FWDSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 *
 * @return Static C string with the short git commit hash, or a "not defined" notice if unavailable at build time.
 */
FWDSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 *
 * @return Static C string with the full git commit hash, or a "not defined" notice if unavailable at build time.
 */
FWDSHARED_EXPORT const char* buildHashLong();
}

#endif // FWD_GLOBAL_H
