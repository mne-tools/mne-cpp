//=============================================================================================================
/**
 * @file     mne_proj_data.cpp
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
 * @brief    Definition of the MneProjData Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_proj_data.h"
#include "mne_surface_old.h"
#include "mne_triangle.h"

#define MALLOC_46(x,t) (t *)malloc((x)*sizeof(t))

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define X_46 0
#define Y_46 1
#define Z_46 2

#define VEC_DOT_46(x,y) ((x)[X_46]*(y)[X_46] + (x)[Y_46]*(y)[Y_46] + (x)[Z_46]*(y)[Z_46])

#define FREE_46(x) if ((char *)(x) != NULL) free((char *)(x))

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneProjData::MneProjData(MNELIB::MneSurfaceOld* s)
{
    int k;
    MneTriangle* tri = Q_NULLPTR;

    a   = MALLOC_46(s->ntri,float);
    b   = MALLOC_46(s->ntri,float);
    c   = MALLOC_46(s->ntri,float);
    act = MALLOC_46(s->ntri,int);

    for (k = 0, tri = s->tris; k < s->ntri; k++, tri++) {
      a[k] =  VEC_DOT_46(tri->r12,tri->r12);
      b[k] =  VEC_DOT_46(tri->r13,tri->r13);
      c[k] =  VEC_DOT_46(tri->r12,tri->r13);

      act[k] = TRUE;
    }
    nactive = s->ntri;
}

//=============================================================================================================

MneProjData::~MneProjData()
{
    FREE_46(a);
    FREE_46(b);
    FREE_46(c);
}
