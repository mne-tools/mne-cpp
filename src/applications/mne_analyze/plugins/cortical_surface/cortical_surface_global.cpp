//=============================================================================================================
/**
 * @file     cortical_surface_global.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    Build-info accessors for the CorticalSurface plugin.
 */

#include "cortical_surface_global.h"

namespace CORTICALSURFACEPLUGIN
{

const char* buildDateTime() { return UTILSLIB::dateTimeNow(); }
const char* buildHash()     { return UTILSLIB::gitHash();     }
const char* buildHashLong() { return UTILSLIB::gitHashLong(); }

} // namespace CORTICALSURFACEPLUGIN
