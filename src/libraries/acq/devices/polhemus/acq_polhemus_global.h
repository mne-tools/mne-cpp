//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_polhemus_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Export macro of mne_acq_polhemus, the Polhemus digitiser bridge of the acquisition library.
 */

#ifndef ACQ_POLHEMUS_GLOBAL_H
#define ACQ_POLHEMUS_GLOBAL_H

#if defined(STATICBUILD)
#define ACQPOLHEMUSSHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_ACQ_POLHEMUS_LIBRARY)
#define ACQPOLHEMUSSHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define ACQPOLHEMUSSHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define ACQPOLHEMUSSHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
#endif

// Exported classes hold Eigen and standard-library members and share their ABI with clients built by the same
// compiler, so MSVC's warnings about members and bases without dll-interface do not apply (Qt disables them alike).
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

#endif // ACQ_POLHEMUS_GLOBAL_H
