//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     mne_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @author   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     August 2012
 * @brief    MNELIB shared-library export/import macros and library build metadata.
 *
 * MNELIB is the central mne-cpp data library: it ties the on-disk FIFF
 * containers exposed by FIFFLIB to the higher-level neuroscience objects
 * (source spaces, hemispheres, BEM surfaces, forward solutions, inverse
 * operators, covariance matrices, raw/epoch/evoked aggregates and the
 * MSH display surfaces consumed by DISP3D). This translation unit only
 * declares the @c MNESHARED_EXPORT visibility macro and the
 * @ref MNELIB::buildDateTime helper so every other compilation unit can
 * resolve symbol visibility identically across static and shared builds.
 */

#ifndef MNE_GLOBAL_H
#define MNE_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QtCore/qglobal.h>
#include <utils/buildinfo.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#  define MNESHARED_EXPORT
#elif defined(MNE_MNE_LIBRARY)
#  define MNESHARED_EXPORT Q_DECL_EXPORT    /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define MNESHARED_EXPORT Q_DECL_IMPORT    /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

//=============================================================================================================
/**
 * @namespace MNELIB
 * @brief     Core MNE data structures (source spaces, source estimates, hemispheres).
 */
namespace MNELIB{

//=============================================================================================================
/**
 * Returns build date and time.
 */
MNESHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 */
MNESHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 */
MNESHARED_EXPORT const char* buildHashLong();
}

#endif // MNE_GLOBAL_H
