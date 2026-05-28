//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mri_global.h
 * @since 2026
 * @date  May 2026
 * @brief Export/import macros and build-info accessors for MRILIB.
 *
 * Provides the @c MRISHARED_EXPORT decorator used by every public
 * symbol in the MRI library so that the same headers can be
 * consumed by a shared-library build (Q_DECL_EXPORT when compiling
 * MNE_MRI_LIBRARY, Q_DECL_IMPORT when linking against it) and a
 * static build (empty decorator). Also exposes @c buildDateTime()
 * and @c buildHash() for runtime provenance reporting --- e.g.
 * when @c mne_analyze prints the linked-library version table
 * in its About dialog or when the registry validator in
 * @c tools/validate_api_registry.py needs to confirm which MRI
 * library snapshot is wired into a deployed binary.
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
 * @brief Volume I/O, voxel geometry and slice resampling for structural MRI data inside mne-cpp.
 *
 * Hosts every public type that participates in loading a 3D MRI volume
 * (MGH/MGZ, NIfTI-1, FreeSurfer COR), describing it in memory
 * (@ref MriVolData, @ref MriSlice), resampling it onto an orthogonal viewing
 * plane (@ref MriSlicer, @ref MriSliceImage) and re-serialising it through
 * the FIFF MRI block hierarchy (@ref MriCorFifIO).
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
