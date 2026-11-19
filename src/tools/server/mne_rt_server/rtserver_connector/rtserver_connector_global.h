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
// QT INCLUDES
//=============================================================================================================

#include <QtGlobal>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define RTSERVER_CONNECTOR_EXPORT
#elif defined(RTSERVER_CONNECTOR_LIBRARY)
#define RTSERVER_CONNECTOR_EXPORT Q_DECL_EXPORT /**< Exported when building the library. */
#else
#define RTSERVER_CONNECTOR_EXPORT Q_DECL_IMPORT /**< Imported by the server and its plugins. */
#endif

#endif // RTSERVER_CONNECTOR_GLOBAL_H
