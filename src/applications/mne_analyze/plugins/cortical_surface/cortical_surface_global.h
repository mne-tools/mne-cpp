//=============================================================================================================
/**
 * @file     cortical_surface_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    Global symbol export macros for the mne_analyze CorticalSurface plugin.
 *
 *           Implements TASK 4.1 (FreeSurfer cortical surface plugin) of the
 *           v2.3.0 release plan.
 */

#ifndef MNEANALYZE_CORTICAL_SURFACE_GLOBAL_H
#define MNEANALYZE_CORTICAL_SURFACE_GLOBAL_H

#include <utils/buildinfo.h>

#include <QtCore/qglobal.h>

#if defined(ANALYZE_CORTICAL_SURFACE_PLUGIN)
#  define CORTICAL_SURFACE_SHARED_EXPORT Q_DECL_EXPORT
#else
#  define CORTICAL_SURFACE_SHARED_EXPORT Q_DECL_IMPORT
#endif

namespace CORTICALSURFACEPLUGIN {

CORTICAL_SURFACE_SHARED_EXPORT const char* buildDateTime();
CORTICAL_SURFACE_SHARED_EXPORT const char* buildHash();
CORTICAL_SURFACE_SHARED_EXPORT const char* buildHashLong();

} // namespace CORTICALSURFACEPLUGIN

#endif // MNEANALYZE_CORTICAL_SURFACE_GLOBAL_H
