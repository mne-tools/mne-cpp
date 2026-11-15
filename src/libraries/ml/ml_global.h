//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     ml_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Export/import macros, build-stamp accessors and namespace anchor for the mne_ml machine-learning library.
 *
 * mne_ml is the thin C++ surface mne-cpp puts around third-party ML
 * runtimes (currently ONNX Runtime) and Python-side training drivers.
 * The library deliberately stays small: an N-dimensional tensor type,
 * an abstract model interface, an ONNX inference backend and a
 * @c QProcess-based trainer wrapper. Anything heavier (graph
 * optimisation, autodiff, GPU kernels) is delegated to the underlying
 * runtime to keep the mne-cpp build tractable on every supported
 * platform, including WebAssembly where the trainer compiles out.
 *
 * This header exposes only the symbol-visibility macro
 * @c MLSHARED_EXPORT and the three build-stamp accessors used by the
 * application about-boxes; pull it from every public declaration so the
 * library can be linked as shared (Qt default), static
 * (@c STATICBUILD) or imported by clients without code changes.
 */

#ifndef ML_GLOBAL_H
#define ML_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/buildinfo.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define MLSHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_ML_LIBRARY)
#define MLSHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define MLSHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define MLSHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
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
 * @return Static C string with the build timestamp.
 */
MLSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 *
 * @return Static C string with the short git commit hash.
 */
MLSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 *
 * @return Static C string with the full git commit hash.
 */
MLSHARED_EXPORT const char* buildHashLong();
}

#endif // ML_GLOBAL_H
