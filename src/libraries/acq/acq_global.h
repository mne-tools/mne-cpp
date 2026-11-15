//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Export macro of mne_acq — Headless acquisition: sources, streams, zero-copy readers and native sample formats.
 *
 * @c ACQSHARED_EXPORT marks the public symbols: exported when building the shared library, imported by
 * its clients, and empty in a static build (@c STATICBUILD).
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

// mne_acq builds without mne_base, so it reports its own build instead of mne::build_date_time() and friends.

/** @return Null-terminated compile date and time of mne_acq. */
ACQSHARED_EXPORT const char* acq_build_date_time();

/** @return Null-terminated short git commit hash mne_acq was built from, or "unknown" in a standalone build. */
ACQSHARED_EXPORT const char* acq_build_hash();

/** @return Null-terminated full git commit hash mne_acq was built from, or "unknown" in a standalone build. */
ACQSHARED_EXPORT const char* acq_build_hash_long();

} // namespace mne

#endif // ACQ_GLOBAL_H
