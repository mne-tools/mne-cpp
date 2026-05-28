//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file ml_global.h
 * @since 2026
 * @date  May 2026
 * @brief Export/import macros, build-stamp accessors and namespace anchor for the MLLIB machine-learning library.
 *
 * MLLIB is the thin C++ surface mne-cpp puts around third-party ML
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

#include <utils/buildinfo.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/qglobal.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#  define MLSHARED_EXPORT
#elif defined(MNE_ML_LIBRARY)
#  define MLSHARED_EXPORT Q_DECL_EXPORT    /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define MLSHARED_EXPORT Q_DECL_IMPORT    /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

//=============================================================================================================
/**
 * @namespace MLLIB
 * @brief Tensors, model abstraction, ONNX Runtime inference and Python training drivers used across mne-cpp.
 */
namespace MLLIB{

//=============================================================================================================
/**
 * Returns build date and time.
 */
MLSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 */
MLSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 */
MLSHARED_EXPORT const char* buildHashLong();
}

#endif // ML_GLOBAL_H
