//=============================================================================================================
/**
 * @file     mne_msh_display_surface.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     April, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Definition of the MneMshDisplaySurface Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_msh_display_surface.h"
#include "mne_surface_old.h"
#include "mne_morph_map.h"
#include "mne_source_space_old.h"

#define FREE_44(x) if ((char *)(x) != NULL) free((char *)(x))

void mne_free_icmatrix_44 (int **m)
{
    if (m) {
        FREE_44(*m);
        FREE_44(m);
    }
}

#define FREE_ICMATRIX_44(m) mne_free_icmatrix_44((m))

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneMshDisplaySurface::MneMshDisplaySurface()
{
    int c;

    filename = Q_NULLPTR;
    time_loaded = 0;
    s = Q_NULLPTR;
    sketch = FALSE;
    color_scale    = Q_NULLPTR;
    vertex_colors  = Q_NULLPTR;
    nvertex_colors = 3;
    marker_colors  = Q_NULLPTR;
    nmarker_colors = 3;
    overlay_values = Q_NULLPTR;
    alt_overlay_values = Q_NULLPTR;
    marker_values      = Q_NULLPTR;
    marker_tri         = Q_NULLPTR;
    marker_tri_no      = Q_NULLPTR;
    nmarker_tri        = 0;
    subj               = Q_NULLPTR;
    surf_name          = Q_NULLPTR;
    rot[0]             = 0.0;
    rot[1]             = 0.0;
    rot[2]             = 0.0;

    move[0]            = 0.0;
    move[1]            = 0.0;
    move[2]            = 0.0;

    eye[0]             = 1.0;
    eye[0]             = 0.0;
    eye[0]             = 0.0;

    up[0]              = 0.0;
    up[1]              = 0.0;
    up[2]              = 1.0;
    fov                = 2.0;
    fov_scale          = 1.0;
    for (c = 0; c < 3; c++) {
      minv[c] = -1.0;
      maxv[c] = 1.0;
    }
    trans          = Q_NULLPTR;
    transparent    = FALSE;
    picked         = Q_NULLPTR;
    npicked        = 0;
    show_aux_data  = FALSE;
    user_data      = Q_NULLPTR;
    user_data_free = Q_NULLPTR;
    maps = Q_NULLPTR;
    nmap = 0;
}

//=============================================================================================================

MneMshDisplaySurface::~MneMshDisplaySurface()
{
    FREE_44(filename);
    delete s;
    FREE_44(marker_values);
    FREE_44(overlay_values);
    FREE_44(alt_overlay_values);
    FREE_44(vertex_colors);
    FREE_44(marker_colors);
    FREE_44(color_scale);
    FREE_ICMATRIX_44(marker_tri);
    FREE_44(marker_tri_no);
    FREE_44(subj);
    FREE_44(surf_name);
    FREE_44(picked);
    if (user_data_free)
      (*user_data_free)(user_data);

    for (int k = 0; k < nmap; k++)
      delete maps[k];
    FREE_44(maps);
    FREE_44(trans);
}

