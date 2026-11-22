//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     connectivity_global.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2016
 * @brief    Export macro of mne_connectivity — Functional connectivity metrics (coherence, PLV, cross-correlation, etc.).
 *
 * @c CONNECTIVITYSHARED_EXPORT marks the public symbols: exported when building the shared library, imported by
 * its clients, and empty in a static build (@c STATICBUILD).
 */

#ifndef CONNECTIVITY_GLOBAL_H
#define CONNECTIVITY_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================


//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define CONNECTIVITYSHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_CONNECTIVITY_LIBRARY)
#define CONNECTIVITYSHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define CONNECTIVITYSHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define CONNECTIVITYSHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
#endif

// Exported classes hold Eigen and standard-library members and share their ABI with clients built by the same
// compiler, so MSVC's warnings about members and bases without dll-interface do not apply (Qt disables them alike).
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

#endif // CONNECTIVITY_GLOBAL_H
