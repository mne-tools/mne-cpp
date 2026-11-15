//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Export macro, build-info accessors and the mne_acq namespace anchor of the acquisition library.
 *
 * mne_acq is the headless acquisition library of MNE-CPP. It depends only on the C++20 standard
 * library, so the export macro is defined here without Qt and the library builds for embedded targets.
 */

#ifndef ACQ_GLOBAL_H
#define ACQ_GLOBAL_H

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define ACQSHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_ACQ_LIBRARY)
#define ACQSHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define ACQSHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define ACQSHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
#endif

// The acq API passes standard-library types (std::string, std::vector, std::span) by value, so a
// shared mne_acq and its clients must be built with the same compiler and C++ runtime. Under that
// requirement MSVC's C4251 (a standard-library member of an exported class has no dll-interface)
// cannot apply; these bracket the exported classes that hold such members.
#if defined(_MSC_VER)
#define ACQ_STD_MEMBERS_BEGIN __pragma(warning(push)) __pragma(warning(disable : 4251))
#define ACQ_STD_MEMBERS_END __pragma(warning(pop))
#else
#define ACQ_STD_MEMBERS_BEGIN
#define ACQ_STD_MEMBERS_END
#endif

namespace mne
{

//=============================================================================================================
/**
 * Returns build date and time.
 *
 * @return Null-terminated string with the compile date and time of the library.
 */
ACQSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 *
 * @return Null-terminated string with the short git commit hash the library was built from.
 */
ACQSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 *
 * @return Null-terminated string with the full git commit hash the library was built from.
 */
ACQSHARED_EXPORT const char* buildHashLong();

} // namespace mne

#endif // ACQ_GLOBAL_H
