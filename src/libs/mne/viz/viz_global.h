//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     viz_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Export macro of mne_viz — what the viewers show and how they behave, without a GUI toolkit.
 *
 * @c VIZSHARED_EXPORT marks the public symbols: exported when building the shared library, imported by
 * its clients, and empty in a static build (@c STATICBUILD).
 */

#ifndef VIZ_GLOBAL_H
#define VIZ_GLOBAL_H

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define VIZSHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_VIZ_LIBRARY)
#define VIZSHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define VIZSHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define VIZSHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
#endif

// Exported classes hold standard-library members and share their ABI with clients built by the same compiler,
// so MSVC's warnings about members and bases without dll-interface do not apply.
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

//=============================================================================================================
/**
 * @namespace mne::viz
 * @brief     Visualization without a GUI toolkit: figures as display lists (figures/), the logic of the trace and
 *            brain views (views/), GPU frame descriptions with a software renderer (gpu/), settings panel
 *            descriptions (forms/) and view models (models/). Toolkit backends such as mne_viz_qt render them.
 */
namespace mne::viz
{
}

#endif // VIZ_GLOBAL_H
