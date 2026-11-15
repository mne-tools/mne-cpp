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
 * @brief    Export macro of mne_fs — FreeSurfer surface, annotation and parcellation I/O for mne-cpp.
 *
 * Houses the readers and in-memory containers for the on-surface side of
 * a FreeSurfer @c recon-all output tree: triangular cortical surfaces
 * (@c lh.pial, @c rh.white, @c lh.inflated, …), per-vertex annotation
 * files (@c lh.aparc.annot, …), the FreeSurferColorLUT-style colour
 * lookup tables they embed, surface labels (@c .label) and the
 * companion lookup against volumetric parcellations
 * (@c aparc+aseg.mgz). The volumetric MRI side lives in MRILIB.
 *
 * @c FSSHARED_EXPORT marks the public symbols: exported when building the shared library, imported by
 * its clients, and empty in a static build (@c STATICBUILD).
 */

#ifndef FS_GLOBAL_H
#define FS_GLOBAL_H


//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define FSSHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_FS_LIBRARY)
#define FSSHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define FSSHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define FSSHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
#endif

// Exported classes hold Eigen and standard-library members and share their ABI with clients built by the same
// compiler, so MSVC's warnings about members and bases without dll-interface do not apply (Qt disables them alike).
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

#endif // FS_GLOBAL_H
