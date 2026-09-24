//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     studio_core_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     March, 2026
 * @version  dev
 * @brief    Defines shared library export macros for the studio core module.
 */

#ifndef MNE_ANALYZE_STUDIO_CORE_GLOBAL_H
#define MNE_ANALYZE_STUDIO_CORE_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(STATICBUILD)
#  define STUDIOCORESHARED_EXPORT
#elif defined(MNE_ANALYZE_STUDIO_CORE_LIBRARY)
#  define STUDIOCORESHARED_EXPORT Q_DECL_EXPORT
#else
#  define STUDIOCORESHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // MNE_ANALYZE_STUDIO_CORE_GLOBAL_H
