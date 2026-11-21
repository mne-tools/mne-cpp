//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     dsp_global.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Christof Pieloth <pieloth@labp.htwk-leipzig.de>
 * @since    0.1.0
 * @date     July 2012
 * @brief    Export macro of mne_dsp — Digital signal processing (filtering, spectrograms, real-time averaging, HPI, noise reduction).
 *
 * @c DSPSHARED_EXPORT marks the public symbols: exported when building the shared library, imported by
 * its clients, and empty in a static build (@c STATICBUILD).
 */

#ifndef DSP_GLOBAL_H
#define DSP_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define DSPSHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_DSP_LIBRARY)
#define DSPSHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define DSPSHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define DSPSHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
#endif

// Exported classes hold Eigen and standard-library members and share their ABI with clients built by the same
// compiler, so MSVC's warnings about members and bases without dll-interface do not apply (Qt disables them alike).
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

namespace mne
{

using mne::StringList;

}

#endif // DSP_GLOBAL_H
