//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mri_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February 2026
 * @brief    Export macro of mne_mri — Volume I/O, voxel geometry and slice resampling for structural MRI data inside mne-cpp.
 *
 * Hosts every public type that participates in loading a 3D MRI volume
 * (MGH/MGZ, NIfTI-1, FreeSurfer COR), describing it in memory
 * (mne::MriVolData, mne::MriSlice), resampling it onto an orthogonal viewing
 * plane (mne::MriSlicer, mne::MriSliceImage) and re-serialising it through
 * the FIFF MRI block hierarchy (mne::MriCorFifIO).
 *
 * @c MRISHARED_EXPORT marks the public symbols: exported when building the shared library, imported by
 * its clients, and empty in a static build (@c STATICBUILD).
 */

#ifndef MRI_GLOBAL_H
#define MRI_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================


//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define MRISHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_MRI_LIBRARY)
#define MRISHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define MRISHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define MRISHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
#endif

// Exported classes hold Eigen and standard-library members and share their ABI with clients built by the same
// compiler, so MSVC's warnings about members and bases without dll-interface do not apply (Qt disables them alike).
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

#endif // MRI_GLOBAL_H
