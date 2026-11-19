//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     pybridge_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Export macro of mne_pybridge — Python interoperability: script launcher and MNE-Python cross-validation helpers.
 *
 * @c PYBRIDGESHARED_EXPORT marks the public symbols: exported when building the shared library, imported by
 * its clients, and empty in a static build (@c STATICBUILD).
 */

#ifndef PYBRIDGE_GLOBAL_H
#define PYBRIDGE_GLOBAL_H


//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define PYBRIDGESHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_PYBRIDGE_LIBRARY)
#define PYBRIDGESHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define PYBRIDGESHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define PYBRIDGESHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
#endif

// Exported classes hold Eigen and standard-library members and share their ABI with clients built by the same
// compiler, so MSVC's warnings about members and bases without dll-interface do not apply (Qt disables them alike).
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

#endif // MNEPYBRIDGE_GLOBAL_H
