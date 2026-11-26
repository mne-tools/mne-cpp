//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     viz_sdl_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Export macro of mne_viz_sdl — the SDL3 backend of mne_viz.
 *
 * @c VIZSDLSHARED_EXPORT marks the public symbols: exported when building the shared library, imported by
 * its clients, and empty in a static build (@c STATICBUILD).
 */

#ifndef VIZ_SDL_VIZ_SDL_GLOBAL_H
#define VIZ_SDL_VIZ_SDL_GLOBAL_H

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define VIZSDLSHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_VIZ_SDL_LIBRARY)
#define VIZSDLSHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define VIZSDLSHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define VIZSDLSHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
#endif

// Exported classes hold standard-library members and share their ABI with clients built by the same compiler,
// so MSVC's warnings about members and bases without dll-interface do not apply.
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

//=============================================================================================================
/**
 * @namespace mne::viz::sdl
 * @brief     SDL3 backend of mne::viz: windows and input from SDL3, frames drawn by the software renderer.
 */
namespace mne::viz::sdl
{
}

#endif // VIZ_SDL_VIZ_SDL_GLOBAL_H
