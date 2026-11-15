//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     channels_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Export macro and build-info accessors of mne_channels.
 *
 * mne_channels holds sensor layouts, channel selections and standard EEG montages.
 */

#ifndef CHANNELS_GLOBAL_H
#define CHANNELS_GLOBAL_H

#include <base/buildinfo.h>

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

namespace mne
{

//=============================================================================================================
/**
 * Returns build date and time.
 *
 * @return Null-terminated string with the library build date and time.
 */
CHANNELSSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 *
 * @return Null-terminated string with the short git commit hash of the build.
 */
CHANNELSSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 *
 * @return Null-terminated string with the full git commit hash of the build.
 */
CHANNELSSHARED_EXPORT const char* buildHashLong();
}

#endif // MNECHANNELS_GLOBAL_H
