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

bool MNEProjectToSurface::nearest_triangle_point(Vector3f r, const int tri, float p, float q, float dist)
{
    //Calculate some helpers
    Vector3f rr = r - this->r1.row(tri).transpose(); //Vector from triangle corner #1 to r
    float v1 = this->r12.row(tri)*rr;
    float v2 = this->r13.row(tri)*rr;

    //Calculate the orthogonal projection of the point r on the plane
    dist = this->nn.row(tri)*rr;
    p = (this->b(tri)*v1 - this->c(tri)*v2)/det(tri);
    q = (this->a(tri)*v2 - this->c(tri)*v1)/det(tri);

    //If the point projects into the triangle we are done
    if (p >= 0.0 && p <= 1.0 && q >= 0.0 && q <= 1.0 && (p+q) <= 1.0)
         {
        return true;
    }

    /*
     * Tough: must investigate the sides
     * We might do something intelligent here. However, for now it is ok
     * to do it in the hard way
     */
    float p0, q0, t0, dist0, best;

    /*
     * Side 1 -> 2
     */
    p0 = p + (q * this->c(tri)) / this->a(tri);
    // Place the point in the corner if it is not on the side
    if (p0 < 0.0)
    {
        p0 = 0.0;
    }
    else if (p0 > 1.0)
    {
        p0 = 1.0;
    }
    // Distance
    dist0 = sqrt((p-p0)*(p-p0)*this->a(tri) +
                 (q-q0)*(q-q0)*this->b(tri) +
                 2*(p-p0)*(q-q0)*this->c(tri) +
                 dist*dist);

    best = dist0;
    p = p0;
    q = q0;
    dist = dist0;

    /*
    * Side 2 -> 3
    */
    t0 = ((a(tri)-c(tri))*(-p) + (b(tri)-c(tri))*q)/(a(tri)+b(tri)-2*c(tri));
    // Place the point in the corner if it is not on the side
    if (t0 < 0.0)
    {
        t0 = 0.0;
    }
    else if (t0 > 1.0)
    {
        t0 = 1.0;
    }
    p0 = 1.0 - t0;
    q0 = t0;
    // Distance
    dist0 = sqrt((p-p0)*(p-p0)*this->a(tri) +
                 (q-q0)*(q-q0)*this->b(tri) +
                 2*(p-p0)*(q-q0)*this->c(tri) +
                 dist*dist);
    if (dist0 < best)
    {
        best = dist0;
        p = p0;
        q = q0;
        dist = dist0;
    }
    /*
    * Side 1 -> 3
    */
    p0 = 0.0;
    q0 = q + (p * c(tri))/b(tri);
    // Place the point in the corner if it is not on the side
    if (q0 < 0.0)
    {
        q0 = 0.0;

    }
    else if (q0 > 1.0)
    {
        q0 = 1.0;
    }
    // Distance
    dist0 = sqrt((p-p0)*(p-p0)*this->a(tri) +
                 (q-q0)*(q-q0)*this->b(tri) +
                 2*(p-p0)*(q-q0)*this->c(tri) +
                 dist*dist);
    if (dist0 < best)
    {
        best = dist0;
        p = p0;
        q = q0;
        dist = dist0;
    }
    return true;
}


//*************************************************************************************************************

void MNEProjectToSurface::project_to_triangle(Vector3f rTri, const float p, const float q, const int tri)
{
    rTri = this->r1.row(tri) + p*this->r12.row(tri) + q*this->r13.row(tri);
    return;
}
