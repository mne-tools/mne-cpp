//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_lsl_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Export macro of mne_acq_lsl.
 */

#ifndef ACQ_LSL_GLOBAL_H
#define ACQ_LSL_GLOBAL_H

#if defined(STATICBUILD)
#define ACQLSLSHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_ACQ_LSL_LIBRARY)
#define ACQLSLSHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define ACQLSLSHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define ACQLSLSHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
#endif

#endif // ACQ_LSL_GLOBAL_H
