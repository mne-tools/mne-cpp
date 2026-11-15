//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     decoding_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Export/import macros and build-info entry points for mne_decoding.
 *
 * mne_decoding groups the supervised- and unsupervised-decomposition
 * algorithms that turn raw M/EEG into discriminative or regressable
 * features for brain-computer interfaces and pattern-recognition
 * pipelines: Common Spatial Patterns (CSP), Source Power Comodulation
 * (SPoC), Spatio-Spectral Decomposition (SSD), and ICA component
 * labelling. All of these algorithms share the same mathematical
 * skeleton — a generalised eigenvalue problem on two covariance
 * matrices — which is why they live in one compilation unit and not in
 * @c rtprocessing or @c dsp.
 *
 * The header only declares the export / import
 * macro plus the build-stamp accessors used by the version probe in the
 * applications layer; the actual algorithms are declared in
 * @ref decoding_csp.h, @ref decoding_spoc.h, @ref decoding_ssd.h and
 * @ref decoding_ica_label.h. Keeping the export macro in its own
 * translation unit lets us link mne_decoding statically (@c STATICBUILD)
 * or as a shared library without touching the per-algorithm files.
 */

#ifndef DECODING_GLOBAL_H
#define DECODING_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/buildinfo.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define DECODINGSHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_DECODING_LIBRARY)
#define DECODINGSHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define DECODINGSHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define DECODINGSHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
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
 * @return Static C string with the compile date and time, e.g. "Sep 30 2026 14:05:12".
 */
DECODINGSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 *
 * @return Static C string with the short git commit hash, or a "not defined" notice if unavailable at build time.
 */
DECODINGSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 *
 * @return Static C string with the full git commit hash, or a "not defined" notice if unavailable at build time.
 */
DECODINGSHARED_EXPORT const char* buildHashLong();
} // namespace mne

#endif // DECODING_GLOBAL_H
