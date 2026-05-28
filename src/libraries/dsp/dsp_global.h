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
 * @c DSPSHARED_EXPORT resolves to @c Q_DECL_EXPORT inside the DSP target
 * itself and to @c Q_DECL_IMPORT for every downstream consumer; it collapses
 * to an empty token in static builds. This macro must annotate every symbol
 * that crosses the shared-library boundary on Windows so the import library
 * is generated correctly.
 */

#ifndef DSP_GLOBAL_H
#define DSP_GLOBAL_H

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
#  define DSPSHARED_EXPORT
#elif defined(MNE_DSP_LIBRARY)
#  define DSPSHARED_EXPORT Q_DECL_EXPORT    /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define DSPSHARED_EXPORT Q_DECL_IMPORT    /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

//=============================================================================================================
/**
 * @namespace DSPLIB
 * @brief     Digital signal processing (filtering, spectrograms, real-time averaging, HPI, noise reduction).
 */
namespace DSPLIB{

//=============================================================================================================
/**
 * Returns build date and time.
 */
DSPSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 */
DSPSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 */
DSPSHARED_EXPORT const char* buildHashLong();
}

#endif // DSP_GLOBAL_H
