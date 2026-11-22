//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     com_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     December, 2012
 * @brief    Export macro of mne_com — Byte transports to devices and servers in standard C++: TCP connections, UDP sockets and serial ports,.
 *
 * each with timeouts and std::error_code results.
 *
 * @c COMSHARED_EXPORT marks the public symbols: exported when building the shared library, imported by
 * its clients, and empty in a static build (@c STATICBUILD).
 */

#ifndef COMMUNICATION_GLOBAL_H
#define COMMUNICATION_GLOBAL_H

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define COMSHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_COM_LIBRARY)
#define COMSHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define COMSHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define COMSHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
#endif

// Exported classes hold Eigen and standard-library members and share their ABI with clients built by the same
// compiler, so MSVC's warnings about members and bases without dll-interface do not apply (Qt disables them alike).
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

#endif // COM_GLOBAL_H
