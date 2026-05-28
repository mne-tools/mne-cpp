//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file field_map.h
 * @since 2026
 * @date  March 2026
 * @brief Forwarding header &mdash; field-map computation moved to the fwd library.
 *
 * Kept as a stub so legacy includes continue to compile. New code
 * should pull in @c <fwd/computeFieldMap.h> directly.
 */

#ifndef FIELD_MAP_H
#define FIELD_MAP_H

#include <fwd/fwd_field_map.h>

namespace DISP3DLIB
{
    /** @deprecated Use FWDLIB::FwdFieldMap instead. */
    using FieldMap = FWDLIB::FwdFieldMap;
}

#endif // FIELD_MAP_H
