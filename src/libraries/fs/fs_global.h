//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fs_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     December 2012
 * @brief    Export/import macros and build-info accessors for the FSLIB FreeSurfer I/O library.
 *
 * Defines the @c FSSHARED_EXPORT visibility macro used by every public
 * class in FSLIB so the library can be built either as a Qt shared
 * library (default) or as a static archive under @c STATICBUILD without
 * source changes. Also exposes the small set of build-info accessors
 * (date/time, abbreviated git hash, full git hash) that downstream
 * tools surface in their About dialogs and log headers.
 */

#ifndef FS_GLOBAL_H
#define FS_GLOBAL_H

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/qglobal.h>
#include <utils/buildinfo.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#  define FSSHARED_EXPORT
#elif defined(MNE_FS_LIBRARY)
#  define FSSHARED_EXPORT Q_DECL_EXPORT    /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define FSSHARED_EXPORT Q_DECL_IMPORT    /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

//=============================================================================================================
/**
 * @namespace FSLIB
 * @brief     FreeSurfer surface, annotation and parcellation I/O for mne-cpp.
 *
 * Houses the readers and in-memory containers for the on-surface side of
 * a FreeSurfer @c recon-all output tree: triangular cortical surfaces
 * (@c lh.pial, @c rh.white, @c lh.inflated, …), per-vertex annotation
 * files (@c lh.aparc.annot, …), the FreeSurferColorLUT-style colour
 * lookup tables they embed, surface labels (@c .label) and the
 * companion lookup against volumetric parcellations
 * (@c aparc+aseg.mgz). The volumetric MRI side lives in MRILIB.
 */
namespace FSLIB{

//=============================================================================================================
/**
 * Returns build date and time.
 */
FSSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 */
FSSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 */
FSSHARED_EXPORT const char* buildHashLong();
}

#endif // FS_GLOBAL_H
