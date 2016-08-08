//=============================================================================================================
/**
* @file     MNEProjectToSurface.cpp
* @author   Jana Kiesel <jana.kiesel@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     Month, Year
*
* @section  LICENSE
*
* Copyright (C) Year, Your name and Matti Hamalainen. All rights reserved.
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
* @brief    MNEProjectToSurface class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_project_to_surface.h"


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_bem.h>
#include <mne/mne_surface.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEProjectToSurface::MNEProjectToSurface()
: r1(MatrixX3f::Zero(1,3))
, r12(MatrixX3f::Zero(1,3))
, r13(MatrixX3f::Zero(1,3))
, nn(MatrixX3f::Zero(1,3))
, a(Vector3f::Zero(3))
, b(Vector3f::Zero(3))
, c(Vector3f::Zero(3))
, det(Vector3f::Zero(3))
{

}


//*************************************************************************************************************

MNEProjectToSurface::MNEProjectToSurface(const MNEBem &p_MNEBem)
{

}


//*************************************************************************************************************

MNEProjectToSurface::MNEProjectToSurface(const MNESurface &p_MNESurf)
{

}


//*************************************************************************************************************

bool MNEProjectToSurface::mne_project_to_surface(const Vector3f r, Vector3f rTri, int bestTri)
{
    return false;
}


//*************************************************************************************************************

bool MNEProjectToSurface::mne_triangle_coords(const Vector3f *r, const int tri, Vector3f *rTri)
{

    return false;
}


//*************************************************************************************************************

bool MNEProjectToSurface::mne_nearest_triangle_point(const Vector3f *r, const int tri, Vector3f *rTri)
{

    return false;
}


//*************************************************************************************************************

bool MNEProjectToSurface::mne_find_closest_on_surface(const MatrixX3f *r, MatrixX3f *rTri, int *nearest, Vector3f *dist)
{

    return false;
}


//*************************************************************************************************************

bool MNEProjectToSurface::nearest_triangle_point(Vector3f r, float p, float q, float dist)
{

    return false;
}


//*************************************************************************************************************

bool MNEProjectToSurface::project_to_triangle(Vector3f rTri, float p, float q)
{
    return false;
}
