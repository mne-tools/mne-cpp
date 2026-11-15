//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     decoding_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Export macro of mne_decoding — Supervised and unsupervised spatial-filter decompositions for M/EEG decoding.
 *
 * DECODINGLIB exposes a small, deliberately scikit-learn-shaped API
 * (@c fit, @c transform, @c fitTransform, @c inverseTransform) around
 * the four spatial-filter families that dominate BCI and biomarker work
 * on continuous and epoched M/EEG: CSP for binary class-discriminative
 * power, SPoC for regressing a continuous target onto band-power, SSD
 * for noise-aware narrowband enhancement, and an ICA component
 * labeller for automatic artefact tagging. The public surface mirrors
 * @c mne.decoding so that a pipeline prototyped in MNE-Python can be
 * ported one-to-one into a real-time mne-cpp application.
 *
 * Every algorithm reduces to a generalised eigendecomposition of two
 * symmetric positive-definite covariance matrices, computed inline with
 * Eigen rather than delegated to LAPACK; this keeps the library free of
 * heavyweight numerical dependencies and makes it trivially usable from
 * the WebAssembly build. MNE-specific extensions on top of the
 * upstream Python API are the explicit @c TransformMode enums
 * (@c AveragePower vs raw @c CspSpace projection), opt-in log or
 * z-score normalisation of band-power features, and a closed-form
 * inverse-transform back to sensor space.
 *
 * @c DECODINGSHARED_EXPORT marks the public symbols: exported when building the shared library, imported by
 * its clients, and empty in a static build (@c STATICBUILD).
 */

#ifndef DECODING_GLOBAL_H
#define DECODING_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================


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

#endif // DECODING_GLOBAL_H
