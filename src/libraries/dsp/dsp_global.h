//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     dsp_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Export/import macros and namespace declaration for the DSP library.
 *
 * The DSP library bundles every digital-signal-processing component shipped
 * with mne-cpp: FIR / IIR / cosine-tapered / Parks–McClellan filter design
 * and application, FFT-based overlap-add convolution, polyphase rational
 * resampling, Morlet wavelet and multitaper time-frequency representations,
 * Welch and multitaper power spectral density estimators, surface Laplacian
 * (CSD) and SPHARA spatial filters, ExtendedInfomax and PICARD ICA, Maxwell
 * filtering (SSS, movement compensation, fine calibration), cHPI line-noise
 * removal, bridged-electrode and LOF bad-channel detection, EOG regression
 * and assorted real-time wrappers (averaging, covariance, noise PSD, inverse
 * operator update).
 *
 * @c DSPSHARED_EXPORT exports the symbols inside the DSP target
 * itself and imports them for every downstream consumer; it collapses
 * to an empty token in static builds. This macro must annotate every symbol
 * that crosses the shared-library boundary on Windows so the import library
 * is generated correctly.
 */

#ifndef DSP_GLOBAL_H
#define DSP_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/buildinfo.h>
#include <base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define DSPSHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_DSP_LIBRARY)
#define DSPSHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define DSPSHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define DSPSHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
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
DSPSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 *
 * @return Static C string with the short git commit hash, or a "not defined" notice if unavailable at build time.
 */
DSPSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 *
 * @return Static C string with the full git commit hash, or a "not defined" notice if unavailable at build time.
 */
DSPSHARED_EXPORT const char* buildHashLong();
}

#endif // DSP_GLOBAL_H
