//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     base_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     March 2013
 * @brief    Export macro and build-info accessors of mne_base.
 *
 * mne_base is the bottom of the library stack: logging, error results, byte streams, JSON, string and path
 * helpers, the thread pool and the circular buffer. It depends on the C++ standard library, zlib and
 * Qt Core only, so every other library can use it.
 */

#ifndef BASE_GLOBAL_H
#define BASE_GLOBAL_H

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define BASESHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_BASE_LIBRARY)
#define BASESHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define BASESHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define BASESHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
#endif

// Exported classes hold Eigen and standard-library members and share their ABI with clients built by the same
// compiler, so MSVC's warnings about members and bases without dll-interface do not apply (Qt disables them alike).
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

//=============================================================================================================
/**
 * @namespace mne
 * @brief     MNE-CPP: reading, processing, source estimation and acquisition of MEG/EEG data in standard C++.
 *
 * Every library shares this namespace; the class names carry their library prefix (FiffInfo, BidsChannel,
 * MnaProject, ...). The Qt-based display libraries live in @ref mne::disp.
 */
namespace mne
{

//=============================================================================================================
/**
 * Returns build date and time.
 *
 * @return Null-terminated string with the library build date and time.
 */
BASESHARED_EXPORT const char* build_date_time();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 *
 * @return Null-terminated string with the short git commit hash of the build.
 */
BASESHARED_EXPORT const char* build_hash();

//=============================================================================================================
/**
 * Returns full build git hash.
 *
 * @return Null-terminated string with the full git commit hash of the build.
 */
BASESHARED_EXPORT const char* build_hash_long();
}

#endif // MNEBASE_GLOBAL_H
