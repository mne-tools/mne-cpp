//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     connectivity_global.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2016
 * @brief    Export/import macros for the @c mne_connectivity functional-connectivity library.
 *
 * @c mne_connectivity implements the spectral and time-domain functional-connectivity
 * estimators used by mne-cpp's source-space and sensor-space analysis
 * pipelines: magnitude-squared coherence and complex coherency, the
 * imaginary part of coherency (Nolte et al., 2004), the phase-locking value
 * (Lachaux et al., 1999), the phase lag index (Stam et al., 2007), the
 * weighted and debiased squared weighted PLI (Vinck et al., 2011), the
 * unbiased squared PLI, Pearson and time-lagged cross-correlation, and the
 * MVAR-based directed metrics Granger causality, DTF and PDC (Baccala &
 * Sameshima, 2001). This header provides the @c CONNECTIVITYSHARED_EXPORT macro that
 * exports the symbols when @c MNE_CONNECTIVITY_LIBRARY is defined during the
 * library build and imports them for downstream consumers, keeping
 * Windows DLL symbol visibility correct without per-class boilerplate.
 */

#ifndef CONNECTIVITY_GLOBAL_H
#define CONNECTIVITY_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/buildinfo.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define CONNECTIVITYSHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_CONNECTIVITY_LIBRARY)
#define CONNECTIVITYSHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define CONNECTIVITYSHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define CONNECTIVITYSHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
#endif

// Exported classes hold Eigen and standard-library members and share their ABI with clients built by the same
// compiler, so MSVC's warnings about members and bases without dll-interface do not apply (Qt disables them alike).
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

namespace mne
{

//=============================================================================================================
/**
 * Returns build date and time.
 *
 * @return Null-terminated build date/time string with static storage duration.
 */
CONNECTIVITYSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 *
 * @return Null-terminated abbreviated git hash string with static storage duration.
 */
CONNECTIVITYSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 *
 * @return Null-terminated full git hash string with static storage duration.
 */
CONNECTIVITYSHARED_EXPORT const char* buildHashLong();
}

#endif // CONNECTIVITY_GLOBAL_H
