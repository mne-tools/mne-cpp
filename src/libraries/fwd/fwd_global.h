//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fwd_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     December 2012
 * @brief    Export macro of mne_fwd — Forward modelling — BEM solver, spherical models, sensor/coil definitions and the lead-field assembly that links current dipoles to MEG/EEG sensor readings.
 *
 * @c FWDSHARED_EXPORT marks the public symbols: exported when building the shared library, imported by
 * its clients, and empty in a static build (@c STATICBUILD).
 */

#ifndef FWD_GLOBAL_H
#define FWD_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/string_utils.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define FWDSHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_FWD_LIBRARY)
#define FWDSHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define FWDSHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define FWDSHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
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

#endif // FWD_GLOBAL_H
