//=============================================================================================================
/**
 * @file     mne_surface_patch.cpp
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
 * @brief    Definition of the MneSurfacePatch Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_surface_patch.h"

#include "mne_source_space_old.h"

#define FREE_49(x) if ((char *)(x) != Q_NULLPTR) free((char *)(x))
#define MALLOC_49(x,t) (t *)malloc((x)*sizeof(t))

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

MneSurfacePatch::MneSurfacePatch(int np)
{
     if (np > 0) {
       vert = MALLOC_49(np,int);
       border = MALLOC_49(np,int);
     }
     else {
       vert = Q_NULLPTR;
       border = Q_NULLPTR;
     }
     s = new MneSourceSpaceOld(np);
     surf_vert = Q_NULLPTR;
     tri       = Q_NULLPTR;
     surf_tri  = Q_NULLPTR;

     np_surf = 0;
     ntri_surf = 0;

     flat  = FALSE;
     user_data = Q_NULLPTR;
     user_data_free = Q_NULLPTR;
}

//=============================================================================================================

MneSurfacePatch::~MneSurfacePatch()
{
    delete s;
    FREE_49(vert);
    FREE_49(border);
    FREE_49(surf_vert);
    FREE_49(tri);
    FREE_49(surf_tri);
    if (user_data && user_data_free)
        user_data_free(user_data);
}
