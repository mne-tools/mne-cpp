//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     disp_widgets_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Export macro of mne_disp_widgets — the Qt Widgets plots, viewers and settings panels on top of mne_disp.
 *
 * @c DISPWIDGETSSHARED_EXPORT marks the public symbols: exported when building the shared library, imported by
 * its clients, and empty in a static build (@c STATICBUILD).
 */

#ifndef DISP_WIDGETS_GLOBAL_H
#define DISP_WIDGETS_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/disp/disp_global.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/qglobal.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define DISPWIDGETSSHARED_EXPORT
#elif defined(MNE_DISP_WIDGETS_LIBRARY)
#define DISPWIDGETSSHARED_EXPORT Q_DECL_EXPORT /**< Exported when compiling the shared library. */
#else
#define DISPWIDGETSSHARED_EXPORT Q_DECL_IMPORT /**< Imported when compiling a client of the shared library. */
#endif

#endif // DISP_WIDGETS_GLOBAL_H
