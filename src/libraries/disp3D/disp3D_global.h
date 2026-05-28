//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 *
 * @file disp3D_global.h
 * @since February 2013
 * @brief Library export/import macros and namespace marker for the disp3D library.
 *
 * disp3D is the Qt-RHI-based 3-D visualisation layer of MNE-CPP. It
 * renders cortical surfaces, BEM heads, source-space points, fitted
 * dipoles, MEG / EEG sensor arrays, digitizer fiducials, connectivity
 * networks and MRI slice planes into a single interactive scene that
 * is shared by every clinical / research GUI in the suite.
 *
 * All renderables go through a uniform pipeline of <em>CPU mesh build</em>
 * &rarr; <em>QRhi vertex / index / instance buffer upload</em> &rarr;
 * <em>shader draw</em>. Per-vertex colour is computed either from
 * FreeSurfer curvature, FsAnnotation parcellation, an iso-contour
 * field map, or a smoothed source-time-course produced by the
 * @ref DISP3DLIB::Interpolation helpers, then packed ABGR and uploaded
 * alongside the geometry so a single draw covers the whole surface.
 *
 * The header itself only carries the import / export macro plumbing;
 * consumers should include @ref BrainView (top-level QWidget) or the
 * scene controllers in scene/ and workers/ for high-level integration.
 */

#ifndef DISP3DLIB_DISP3D_GLOBAL_H
#define DISP3DLIB_DISP3D_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QtCore/qglobal.h>
#include <utils/buildinfo.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#  define DISP3DSHARED_EXPORT
#elif defined(MNE_DISP3D_LIBRARY)
#  define DISP3DSHARED_EXPORT Q_DECL_EXPORT    /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define DISP3DSHARED_EXPORT Q_DECL_IMPORT    /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

//=============================================================================================================
/**
 * @namespace DISP3DLIB
 * @brief     3-D brain visualisation using the Qt RHI rendering backend.
 */
namespace DISP3DLIB{

//=============================================================================================================
/**
 * Returns build date and time.
 */
const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 */
const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 */
const char* buildHashLong();
}

#endif // DISP3DLIB_DISP3D_GLOBAL_H
