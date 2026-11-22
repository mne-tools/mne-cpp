//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     channels_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Export macro of mne_channels — Sensor layouts, channel selections and standard EEG montages.
 *
 * @c CHANNELSSHARED_EXPORT marks the public symbols: exported when building the shared library, imported by
 * its clients, and empty in a static build (@c STATICBUILD).
 */

#ifndef CHANNELS_GLOBAL_H
#define CHANNELS_GLOBAL_H


//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define CHANNELSSHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_CHANNELS_LIBRARY)
#define CHANNELSSHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define CHANNELSSHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define CHANNELSSHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
#endif

// Exported classes hold Eigen and standard-library members and share their ABI with clients built by the same
// compiler, so MSVC's warnings about members and bases without dll-interface do not apply (Qt disables them alike).
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

#endif // MNECHANNELS_GLOBAL_H
