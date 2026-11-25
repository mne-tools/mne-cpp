//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     scene_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Export macro of mne_scene — figures as display lists, rendered without a window system.
 *
 * @c SCENESHARED_EXPORT marks the public symbols: exported when building the shared library, imported by
 * its clients, and empty in a static build (@c STATICBUILD).
 */

#ifndef SCENE_SCENE_GLOBAL_H
#define SCENE_SCENE_GLOBAL_H

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define SCENESHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_SCENE_LIBRARY)
#define SCENESHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define SCENESHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define SCENESHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
#endif

// Exported classes hold standard-library members and share their ABI with clients built by the same compiler,
// so MSVC's warnings about members and bases without dll-interface do not apply.
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

#endif // SCENE_SCENE_GLOBAL_H
