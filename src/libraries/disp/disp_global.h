//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     disp_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     August 2012
 * @brief    Export macro of mne_disp — 2-D display widgets and visualisation helpers (charts, topography, colour maps).
 *
 * @c DISPSHARED_EXPORT marks the public symbols: exported when building the shared library, imported by
 * its clients, and empty in a static build (@c STATICBUILD).
 */

#ifndef DISP_GLOBAL_H
#define DISP_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/qglobal.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define DISPSHARED_EXPORT
#elif defined(MNE_DISP_LIBRARY)
#define DISPSHARED_EXPORT Q_DECL_EXPORT /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#define DISPSHARED_EXPORT Q_DECL_IMPORT /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

//=============================================================================================================
/**
 * @namespace mne::disp
 * @brief     Qt-based 2D and 3D display: plots, viewers and the 3D brain view (mne_disp and mne_disp3D).
 */
namespace mne::disp
{

using mne::StringList;

}

#endif // DISP_GLOBAL_H
