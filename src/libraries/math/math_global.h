//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     math_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Export/import macros and build-stamp accessors for mne_math.
 *
 * mne_math is the numerical-helpers layer of mne-cpp: linear algebra
 * (@ref mne::Linalg), general numerics and baseline rescaling
 * (@ref mne::Numerics), K-means clustering (@ref mne::KMeans),
 * multi-taper spectral estimation (@ref mne::Spectral),
 * Nelder–Mead simplex optimisation
 * (@ref mne::SimplexAlgorithm), sphere fitting (@ref mne::Sphere)
 * thin-plate-spline warping (@ref mne::Warp) and plain-text matrix files
 * (@ref mne::write_matrix, @ref mne::read_matrix). It depends only on
 * Eigen and mne_base and is therefore safe to pull into any
 * higher-level library (mne_fiff, mne_fwd, mne_inv, mne_connectivity).
 *
 * This header defines the @c MATHSHARED_EXPORT visibility macro that
 * toggles between symbol export and import depending on
 * whether the library itself or a consumer is being compiled, plus the
 * @c STATICBUILD short-circuit used on WebAssembly and other static
 * deployments. It also exposes @c buildDateTime(), @c buildHash() and
 * @c buildHashLong() so applications can stamp produced files with the
 * exact mne_math revision they were linked against.
 */

#ifndef MATH_GLOBAL_H
#define MATH_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/buildinfo.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define MATHSHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_MATH_LIBRARY)
#define MATHSHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define MATHSHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define MATHSHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
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
MATHSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 *
 * @return Static C string with the short git commit hash, or a "not defined" notice if unavailable at build time.
 */
MATHSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 *
 * @return Static C string with the full git commit hash, or a "not defined" notice if unavailable at build time.
 */
MATHSHARED_EXPORT const char* buildHashLong();
}

#endif // MATH_GLOBAL_H
