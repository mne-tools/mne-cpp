//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     August 2012
 * @brief    Export macro of mne_fiff — FIFF file I/O, in-memory data structures and high-level readers/writers.
 *
 * Houses every class that round-trips an Elekta/Neuromag FIFF file:
 * the stream layer (mne::FiffStream, mne::FiffTag, mne::FiffDirNode,
 * mne::FiffDirEntry), the measurement metadata (mne::FiffInfo,
 * mne::FiffChInfo, mne::FiffCoordTrans, mne::FiffId, mne::FiffDigPoint),
 * the data containers (mne::FiffRawData, mne::FiffEvoked,
 * mne::FiffEvokedSet, mne::FiffCov, mne::FiffProj, mne::FiffCtfComp,
 * mne::FiffNamedMatrix, mne::FiffSparseMatrix) and the convenience facade
 * @c FIFF. Surface-level parity with @c mne.io.fiff in MNE-Python.
 *
 * @c FIFFSHARED_EXPORT marks the public symbols: exported when building the shared library, imported by
 * its clients, and empty in a static build (@c STATICBUILD).
 */

#ifndef FIFF_GLOBAL_H
#define FIFF_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/string_utils.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define FIFFSHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_FIFF_LIBRARY)
#define FIFFSHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define FIFFSHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define FIFFSHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
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

#endif // FIFF_GLOBAL_H
