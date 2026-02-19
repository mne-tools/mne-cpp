//=============================================================================================================
/**
 * @file     field_map.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     January, 2026
 *
 * @brief    Forwarding header – field mapping moved to fwd library.
 *
 * This header is kept for backward compatibility. New code should include
 * <fwd/fwd_field_map.h> directly and use FWDLIB::FwdFieldMap.
 */

#ifndef FIELD_MAP_H
#define FIELD_MAP_H

#include <fwd/fwd_field_map.h>

namespace DISP3DRHILIB
{
    /** @deprecated Use FWDLIB::FwdFieldMap instead. */
    using FieldMap = FWDLIB::FwdFieldMap;
}

#endif // FIELD_MAP_H
