//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Export macro of mne_bids — BIDS dataset reading, writing, path construction, and sidecar metadata handling for iEEG/EEG/MEG.
 *
 * @c BIDSSHARED_EXPORT marks the public symbols: exported when building the shared library, imported by
 * its clients, and empty in a static build (@c STATICBUILD).
 */

#ifndef BIDS_GLOBAL_H
#define BIDS_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================


//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define BIDSSHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_BIDS_LIBRARY)
#define BIDSSHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define BIDSSHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define BIDSSHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
#endif

// Exported classes hold Eigen and standard-library members and share their ABI with clients built by the same
// compiler, so MSVC's warnings about members and bases without dll-interface do not apply (Qt disables them alike).
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

#endif // BIDS_GLOBAL_H
