//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file math_global.h
 * @since 2026
 * @date  March 2026
 * @brief Export/import macros and build-stamp accessors for MATHLIB.
 *
 * MATHLIB is the numerical-helpers layer of mne-cpp: linear algebra
 * (@ref UTILSLIB::Linalg), general numerics and baseline rescaling
 * (@ref UTILSLIB::Numerics), K-means clustering (@ref UTILSLIB::KMeans),
 * multi-taper spectral estimation (@ref UTILSLIB::Spectral),
 * Nelder–Mead simplex optimisation
 * (@ref UTILSLIB::SimplexAlgorithm), sphere fitting (@ref UTILSLIB::Sphere)
 * and thin-plate-spline warping (@ref UTILSLIB::Warp). It depends only on
 * Eigen, Qt core and UTILSLIB and is therefore safe to pull into any
 * higher-level library (FIFFLIB, FWDLIB, INVERSELIB, CONNECTIVITYLIB).
 *
 * This header defines the @c MATHSHARED_EXPORT visibility macro that
 * toggles between @c Q_DECL_EXPORT and @c Q_DECL_IMPORT depending on
 * whether the library itself or a consumer is being compiled, plus the
 * @c STATICBUILD short-circuit used on WebAssembly and other static
 * deployments. It also exposes @c buildDateTime(), @c buildHash() and
 * @c buildHashLong() so applications can stamp produced files with the
 * exact MATHLIB revision they were linked against.
 */

#ifndef MATH_GLOBAL_H
#define MATH_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/buildinfo.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/qglobal.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#  define MATHSHARED_EXPORT
#elif defined(MNE_MATH_LIBRARY)
#  define MATHSHARED_EXPORT Q_DECL_EXPORT    /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define MATHSHARED_EXPORT Q_DECL_IMPORT    /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

//=============================================================================================================
/**
 * @namespace MATHLIB
 * @brief     Mathematical algorithms and geometry (linear algebra, optimization, spectral estimation).
 */
namespace MATHLIB{

//=============================================================================================================
/**
 * Returns build date and time.
 */
MATHSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 */
MATHSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 */
MATHSHARED_EXPORT const char* buildHashLong();
}

#endif // MATH_GLOBAL_H
