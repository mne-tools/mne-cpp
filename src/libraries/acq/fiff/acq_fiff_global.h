//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_fiff_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Export macro of mne_acq_fiff, the FIFF bridge of the acquisition library.
 */

#ifndef ACQ_FIFF_GLOBAL_H
#define ACQ_FIFF_GLOBAL_H

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define ACQFIFFSHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_ACQ_FIFF_LIBRARY)
#define ACQFIFFSHARED_EXPORT __declspec(dllexport) /**< Exported when building mne_acq_fiff. */
#else
#define ACQFIFFSHARED_EXPORT __declspec(dllimport) /**< Imported by clients of mne_acq_fiff. */
#endif
#else
#define ACQFIFFSHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
#endif

#endif // ACQ_FIFF_GLOBAL_H
