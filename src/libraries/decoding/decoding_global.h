//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file decoding_global.h
 * @since 2026
 * @date  May 2026
 * @brief Export/import macros and build-info entry points for DECODINGLIB.
 *
 * DECODINGLIB groups the supervised- and unsupervised-decomposition
 * algorithms that turn raw M/EEG into discriminative or regressable
 * features for brain-computer interfaces and pattern-recognition
 * pipelines: Common Spatial Patterns (CSP), Source Power Comodulation
 * (SPoC), Spatio-Spectral Decomposition (SSD), and ICA component
 * labelling. All of these algorithms share the same mathematical
 * skeleton — a generalised eigenvalue problem on two covariance
 * matrices — which is why they live in one compilation unit and not in
 * @c rtprocessing or @c dsp.
 *
 * The header only declares the Qt @c Q_DECL_EXPORT / @c Q_DECL_IMPORT
 * macro plus the build-stamp accessors used by the version probe in the
 * applications layer; the actual algorithms are declared in
 * @ref decoding_csp.h, @ref decoding_spoc.h, @ref decoding_ssd.h and
 * @ref decoding_ica_label.h. Keeping the export macro in its own
 * translation unit lets us link DECODINGLIB statically (@c STATICBUILD)
 * or as a shared library without touching the per-algorithm files.
 */

#ifndef DECODING_GLOBAL_H
#define DECODING_GLOBAL_H

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
#  define DECODINGSHARED_EXPORT
#elif defined(MNE_DECODING_LIBRARY)
#  define DECODINGSHARED_EXPORT Q_DECL_EXPORT    /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define DECODINGSHARED_EXPORT Q_DECL_IMPORT    /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

//=============================================================================================================
/**
 * @namespace DECODINGLIB
 * @brief     Supervised and unsupervised spatial-filter decompositions for M/EEG decoding.
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
 */
namespace DECODINGLIB{

//=============================================================================================================
/**
 * Returns build date and time.
 */
DECODINGSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 */
DECODINGSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 */
DECODINGSHARED_EXPORT const char* buildHashLong();
} // namespace DECODINGLIB

#endif // DECODING_GLOBAL_H
