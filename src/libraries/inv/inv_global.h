//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     inv_global.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February 2013
 * @brief    Export macro of mne_inv — Inverse source estimation (MNE, dSPM, sLORETA, dipole fitting).
 *
 * @c INVSHARED_EXPORT marks the public symbols: exported when building the shared library, imported by
 * its clients, and empty in a static build (@c STATICBUILD).
 */

#ifndef INV_GLOBAL_H
#define INV_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/string_utils.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define INVSHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_INV_LIBRARY)
#define INVSHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define INVSHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define INVSHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
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

#endif // INV_GLOBAL_H
