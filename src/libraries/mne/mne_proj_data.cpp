//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_proj_data.cpp
 * @since 2026
 * @date  April 2026
 * @brief Implementation of @ref MNELIB::MNEProjData.
 *
 * Implements lifecycle, channel-list assembly and the on-demand rebuild
 * of the active projection operator against the current sensor set.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_proj_data.h"
#include "mne_surface.h"
#include "mne_triangle.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEProjData::MNEProjData(const MNELIB::MNESurface* s)
{
    a.resize(s->ntri);
    b.resize(s->ntri);
    c.resize(s->ntri);
    act.resize(s->ntri);

    const MNETriangle* tri = s->tris.data();
    for (int k = 0; k < s->ntri; k++, tri++) {
      a[k] =  tri->r12.dot(tri->r12);
      b[k] =  tri->r13.dot(tri->r13);
      c[k] =  tri->r12.dot(tri->r13);

      act[k] = 1;
    }
    nactive = s->ntri;
}
