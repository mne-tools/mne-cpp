//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rtserver_connector_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Export macro of the mne_rt_server connector interface library.
 */

#ifndef RTSERVER_CONNECTOR_GLOBAL_H
#define RTSERVER_CONNECTOR_GLOBAL_H

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define RTSERVER_CONNECTOR_EXPORT
#elif defined(_WIN32)
#if defined(RTSERVER_CONNECTOR_LIBRARY)
#define RTSERVER_CONNECTOR_EXPORT __declspec(dllexport) /**< Exported when building the library. */
#else
#define RTSERVER_CONNECTOR_EXPORT __declspec(dllimport) /**< Imported by the server and its connectors. */
#endif
#else
#define RTSERVER_CONNECTOR_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
#endif

// Exported classes hold standard-library members and share their ABI with clients built by the same compiler.
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

#endif // RTSERVER_CONNECTOR_GLOBAL_H
