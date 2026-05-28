// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2026 MNE-CPP Authors
//   Christoph Dinh <christoph.dinh@mne-cpp.org>
//   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
//   Juan GPC <jgarciaprieto@mgh.harvard.edu>
//   Gabriel B Motta <gbmotta@mgh.harvard.edu>
//   Gabriel Motta <gabrielbenmotta@gmail.com>

//=============================================================================================================
/**
 * @file     mri_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 *
 * @brief    Export/import macros and build-info accessors for MRILIB.
 *
 *           Provides the @c MRISHARED_EXPORT decorator used by every public
 *           symbol in the MRI library so that the same headers can be
 *           consumed by a shared-library build (Q_DECL_EXPORT when compiling
 *           MNE_MRI_LIBRARY, Q_DECL_IMPORT when linking against it) and a
 *           static build (empty decorator). Also exposes @c buildDateTime()
 *           and @c buildHash() for runtime provenance reporting --- e.g.
 *           when @c mne_analyze prints the linked-library version table
 *           in its About dialog or when the registry validator in
 *           @c tools/validate_api_registry.py needs to confirm which MRI
 *           library snapshot is wired into a deployed binary.
 */

#ifndef MRI_GLOBAL_H
#define MRI_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QtCore/qglobal.h>
#include <utils/buildinfo.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#  define MRISHARED_EXPORT
#elif defined(MNE_MRI_LIBRARY)
#  define MRISHARED_EXPORT Q_DECL_EXPORT    /**< Shared library export. */
#else
#  define MRISHARED_EXPORT Q_DECL_IMPORT    /**< Shared library import. */
#endif

//=============================================================================================================
/**
 * @namespace MRILIB
 * @brief     MRI volume and coordinate-system I/O (volumes, voxel geometry, transforms).
 */
namespace MRILIB {

//=============================================================================================================
/**
 * Returns the build date and time.
 */
MRISHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns the short git hash.
 */
MRISHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns the long git hash.
 */
MRISHARED_EXPORT const char* buildHashLong();

} // namespace MRILIB

#endif // MRI_GLOBAL_H
