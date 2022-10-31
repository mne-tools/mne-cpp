//=============================================================================================================
/**
 * @file     dipole_forward.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     December, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the DipoleForward Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dipole_forward.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace INVERSELIB;

#define FREE_CMATRIX_4(m) mne_free_cmatrix_4((m))
#define FREE_4(x) if ((char *)(x) != NULL) free((char *)(x))

void mne_free_cmatrix_4 (float **m)
{
    if (m) {
        FREE_4(*m);
        FREE_4(m);
    }
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DipoleForward::DipoleForward()
: rd(NULL)
, fwd(NULL)
, scales(NULL)
, uu(NULL)
, vv(NULL)
, sing(NULL)
, nch(0)
, ndip(0)
{
}

////=============================================================================================================

//DipoleForward::DipoleForward(const DipoleForward& p_DipoleForward)
//{
//}

//=============================================================================================================

DipoleForward::~DipoleForward()
{
    if(rd)
        FREE_CMATRIX_4(rd);
    if(fwd)
        FREE_CMATRIX_4(fwd);
    if(uu)
        FREE_CMATRIX_4(uu);
    if(vv)
        FREE_CMATRIX_4(vv);
    if(sing)
        FREE_4(sing);
    if(scales)
        FREE_4(scales);
}
