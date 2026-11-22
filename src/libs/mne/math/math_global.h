//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     math_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Export macro of mne_math — Mathematical algorithms and geometry (linear algebra, optimization, spectral estimation).
 *
 * @c MATHSHARED_EXPORT marks the public symbols: exported when building the shared library, imported by
 * its clients, and empty in a static build (@c STATICBUILD).
 */

#ifndef MATH_GLOBAL_H
#define MATH_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================


//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define MATHSHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_MATH_LIBRARY)
#define MATHSHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define MATHSHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define MATHSHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
#endif

// Exported classes hold Eigen and standard-library members and share their ABI with clients built by the same
// compiler, so MSVC's warnings about members and bases without dll-interface do not apply (Qt disables them alike).
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

#endif // MATH_GLOBAL_H
