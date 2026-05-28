//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_global.h
 * @since March 2026
 * @brief INVLIB library export/import macros, build-info accessors, and namespace docstring for the inverse-solver library.
 *
 * The INVLIB library bundles every inverse-method, beamformer and HPI-fit
 * class used by mne-cpp: minimum-norm / dSPM / sLORETA / eLORETA, LCMV and
 * DICS beamformers, RAP-MUSIC, TRAP-MUSIC, MxNE / TF-MxNE / Gamma-MAP,
 * contextual MNE (CMNE), ECD dipole fitting and continuous head-position
 * estimation. This header defines the @c INVSHARED_EXPORT macro that
 * controls symbol visibility for the shared/static build variants, declares
 * the @c INVLIB namespace, and exposes @ref buildDateTime and
 * @ref buildHash so applications can stamp the linked INVLIB version into
 * log files and reports.
 */

#ifndef INV_GLOBAL_H
#define INV_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QtCore/qglobal.h>
#include <utils/buildinfo.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#  define INVSHARED_EXPORT
#elif defined(MNE_INV_LIBRARY)
#  define INVSHARED_EXPORT Q_DECL_EXPORT    /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define INVSHARED_EXPORT Q_DECL_IMPORT    /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

//=============================================================================================================
/**
 * @namespace INVLIB
 * @brief     Inverse source estimation (MNE, dSPM, sLORETA, dipole fitting).
 */
namespace INVLIB{

//=============================================================================================================
/**
 * Returns build date and time.
 */
INVSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 */
INVSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 */
INVSHARED_EXPORT const char* buildHashLong();

}

#endif // INV_GLOBAL_H
