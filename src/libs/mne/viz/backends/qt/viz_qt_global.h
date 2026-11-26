//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     viz_qt_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     August 2012
 * @brief    Export macro of mne_viz_qt — the Qt backend of mne_viz: QRhi rendering, windows, widgets and item models.
 *
 * @c VIZQTSHARED_EXPORT marks the public symbols: exported when building the shared library, imported by
 * its clients, and empty in a static build (@c STATICBUILD).
 */

#ifndef VIZ_QT_VIZ_QT_GLOBAL_H
#define VIZ_QT_VIZ_QT_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/qglobal.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define VIZQTSHARED_EXPORT
#elif defined(MNE_VIZ_QT_LIBRARY)
#define VIZQTSHARED_EXPORT Q_DECL_EXPORT /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#define VIZQTSHARED_EXPORT Q_DECL_IMPORT /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

//=============================================================================================================
/**
 * @namespace mne::viz::qt
 * @brief     Qt backend of mne::viz: QRhi rendering of GPU frames, QWindow hosts, 2-D plots and viewers (plots/,
 *            viewers/), the 3-D brain view (3d/) and Qt item models adapting the view models of mne::viz.
 */
namespace mne::viz::qt
{

using mne::StringList;

}

#endif // VIZ_QT_VIZ_QT_GLOBAL_H
