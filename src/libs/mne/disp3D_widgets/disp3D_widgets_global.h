//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     disp3D_widgets_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Export macro of mne_disp3D_widgets — the QRhiWidget host of the 3-D brain view.
 *
 * @c DISP3DWIDGETSSHARED_EXPORT marks the public symbols: exported when building the shared library, imported by
 * its clients, and empty in a static build (@c STATICBUILD).
 */

#ifndef DISP3D_WIDGETS_GLOBAL_H
#define DISP3D_WIDGETS_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/disp3D/disp3D_global.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/qglobal.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define DISP3DWIDGETSSHARED_EXPORT
#elif defined(MNE_DISP3D_WIDGETS_LIBRARY)
#define DISP3DWIDGETSSHARED_EXPORT Q_DECL_EXPORT /**< Exported when compiling the shared library. */
#else
#define DISP3DWIDGETSSHARED_EXPORT Q_DECL_IMPORT /**< Imported when compiling a client of the shared library. */
#endif

#endif // DISP3D_WIDGETS_GLOBAL_H
