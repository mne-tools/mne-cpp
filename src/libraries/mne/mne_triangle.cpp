//=============================================================================================================
/**
 * @file     mne_triangle.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the MneTriangle Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_triangle.h"

#define X_50 0
#define Y_50 1
#define Z_50 2

#define VEC_DOT_50(x,y) ((x)[X_50]*(y)[X_50] + (x)[Y_50]*(y)[Y_50] + (x)[Z_50]*(y)[Z_50])
#define VEC_LEN_50(x) sqrt(VEC_DOT_50(x,x))

#define VEC_DIFF_50(from,to,diff) {\
    (diff)[X_50] = (to)[X_50] - (from)[X_50];\
    (diff)[Y_50] = (to)[Y_50] - (from)[Y_50];\
    (diff)[Z_50] = (to)[Z_50] - (from)[Z_50];\
    }

#define CROSS_PRODUCT_50(x,y,xy) {\
    (xy)[X_50] =   (x)[Y_50]*(y)[Z_50]-(y)[Y_50]*(x)[Z_50];\
    (xy)[Y_50] = -((x)[X_50]*(y)[Z_50]-(y)[X_50]*(x)[Z_50]);\
    (xy)[Z_50] =   (x)[X_50]*(y)[Y_50]-(y)[X_50]*(x)[Y_50];\
    }

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneTriangle::MneTriangle()
{
}

//=============================================================================================================

MneTriangle::~MneTriangle()
{
}

//=============================================================================================================

void MneTriangle::compute_data()
/*
 * Normal vector of a triangle and other stuff
 */
{
    float size,sizey;
    int   c;
    VEC_DIFF_50 (r1,r2,r12);
    VEC_DIFF_50 (r1,r3,r13);
    CROSS_PRODUCT_50 (r12,r13,nn);
    size = VEC_LEN_50(nn);
    /*
        * Possibly zero area triangles
        */
    if (size > 0) {
        nn[X_50] = nn[X_50]/size;
        nn[Y_50] = nn[Y_50]/size;
        nn[Z_50] = nn[Z_50]/size;
    }
    area = size/2.0;
    sizey = VEC_LEN_50(r13);
    if (sizey <= 0)
        sizey = 1.0;

    for (c = 0; c < 3; c++) {
        ey[c] = r13[c]/sizey;
        cent[c] = (r1[c]+r2[c]+r3[c])/3.0;
    }
    CROSS_PRODUCT_50(ey,nn,ex);

    return;
}
