//=============================================================================================================
/**
 * @file     mne_project_to_surface.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     August, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch, Matti Hamalainen. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_project_to_surface.h"

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_bem_surface.h>
#include <mne/mne_surface.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Geometry>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEProjectToSurface::MNEProjectToSurface()
: r1(MatrixX3f::Zero(1,3))
, r12(MatrixX3f::Zero(1,3))
, r13(MatrixX3f::Zero(1,3))
, nn(MatrixX3f::Zero(1,3))
, a(VectorXf::Zero(1))
, b(VectorXf::Zero(1))
, c(VectorXf::Zero(1))
, det(VectorXf::Zero(1))
{
}

//=============================================================================================================

MNEProjectToSurface::MNEProjectToSurface(const MNEBemSurface &p_MNEBemSurf)
: r1(MatrixX3f::Zero(p_MNEBemSurf.ntri,3))
, r12(MatrixX3f::Zero(p_MNEBemSurf.ntri,3))
, r13(MatrixX3f::Zero(p_MNEBemSurf.ntri,3))
, nn(MatrixX3f::Zero(p_MNEBemSurf.ntri,3))
, a(VectorXf::Zero(p_MNEBemSurf.ntri))
, b(VectorXf::Zero(p_MNEBemSurf.ntri))
, c(VectorXf::Zero(p_MNEBemSurf.ntri))
, det(VectorXf::Zero(p_MNEBemSurf.ntri))
{
    for (int i = 0; i < p_MNEBemSurf.ntri; ++i)
    {
        r1.row(i) = p_MNEBemSurf.rr.row(p_MNEBemSurf.tris(i,0));
        r12.row(i) = p_MNEBemSurf.rr.row(p_MNEBemSurf.tris(i,1)) - r1.row(i);
        r13.row(i) = p_MNEBemSurf.rr.row(p_MNEBemSurf.tris(i,2)) - r1.row(i);
        a(i) = r12.row(i) * r12.row(i).transpose();
        b(i) = r13.row(i) * r13.row(i).transpose();
        c(i) = r12.row(i) * r13.row(i).transpose();
    }

    if (!(p_MNEBemSurf.tri_nn.isZero(0)))
    {
        nn = p_MNEBemSurf.tri_nn.cast<float>();
    }
    else
    {
        for (int i = 0; i < p_MNEBemSurf.ntri; ++i)
        {
            nn.row(i) = r12.row(i).transpose().cross(r13.row(i).transpose()).transpose();
        }
    }
    det = (a.array()*b.array() - c.array()*c.array()).matrix();
}

//=============================================================================================================

MNEProjectToSurface::MNEProjectToSurface(const MNESurface &p_MNESurf)
: r1(MatrixX3f::Zero(p_MNESurf.ntri,3))
, r12(MatrixX3f::Zero(p_MNESurf.ntri,3))
, r13(MatrixX3f::Zero(p_MNESurf.ntri,3))
, nn(MatrixX3f::Zero(p_MNESurf.ntri,3))
, a(VectorXf::Zero(p_MNESurf.ntri))
, b(VectorXf::Zero(p_MNESurf.ntri))
, c(VectorXf::Zero(p_MNESurf.ntri))
, det(VectorXf::Zero(p_MNESurf.ntri))
{
    for (int i = 0; i < p_MNESurf.ntri; ++i)
    {
        r1.row(i) = p_MNESurf.rr.row(p_MNESurf.tris(i,0));
        r12.row(i) = p_MNESurf.rr.row(p_MNESurf.tris(i,1)) - r1.row(i);
        r13.row(i) = p_MNESurf.rr.row(p_MNESurf.tris(i,2)) - r1.row(i);
        nn.row(i) = r12.row(i).transpose().cross(r13.row(i).transpose()).transpose();
        a(i) = r12.row(i) * r12.row(i).transpose();
        b(i) = r13.row(i) * r13.row(i).transpose();
        c(i) = r12.row(i) * r13.row(i).transpose();
    }

    det = (a.array()*b.array() - c.array()*c.array()).matrix();
}

//=============================================================================================================

bool MNEProjectToSurface::mne_find_closest_on_surface(const MatrixXf &r, const int np, MatrixXf &rTri,
                                                      VectorXi &nearest, VectorXf &dist)
{
    // resize output
    nearest.resize(np);
    dist.resize(np);
    rTri.resize(np,3);

    if (this->r1.isZero(0))
    {
        qDebug() << "No surface loaded to make the projection./n";
        return false;
    }
    int bestTri = -1;
    float bestDist = -1;
    Vector3f rTriK;
    for (int k = 0; k < np; ++k)
    {
        /*
         * To do: decide_search_restriction for the use in an iterative closest point to plane algorithm
         * For now it's OK to go through all triangles.
         */
        if (!this->mne_project_to_surface(r.row(k).transpose(), rTriK, bestTri, bestDist))
        {
            qDebug() << "The projection of point number " << k << " didn't work./n";
            return false;
        }
        rTri.row(k) = rTriK.transpose();
        nearest[k] = bestTri;
        dist[k] = bestDist;
    }
    return true;
}

//=============================================================================================================

bool MNEProjectToSurface::mne_project_to_surface(const Vector3f &r, Vector3f &rTri, int &bestTri, float &bestDist)
{
    float p = 0, q = 0, p0 = 0, q0 = 0, dist0 = 0;
    bestDist = 0.0f;
    bestTri = -1;
    for (int tri = 0; tri < a .size(); ++tri)
    {
        if (!this->nearest_triangle_point(r, tri, p0, q0, dist0))
        {
            qDebug() << "The projection on triangle " << tri << " didn't work./n";
            return false;
        }

        if ((bestTri < 0) || (std::fabs(dist0) < std::fabs(bestDist)))
        {
            bestDist = dist0;
            p = p0;
            q = q0;
            bestTri = tri;
        }
    }

    if (bestTri >= 0)
    {
        if (!this->project_to_triangle(rTri, p, q, bestTri))
        {
            qDebug() << "The coordinate transform to cartesian system didn't work./n";
            return false;
        }
        return true;
    }

    qDebug() << "No best Triangle found./n";
    return false;
}

//=============================================================================================================

bool MNEProjectToSurface::nearest_triangle_point(const Vector3f &r, const int tri, float &p, float &q, float &dist)
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
    float p0, q0, t0, dist0, best, bestp, bestq;

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
    q0 = 0;
    // Distance
    dist0 = sqrt((p-p0)*(p-p0)*this->a(tri) +
                 (q-q0)*(q-q0)*this->b(tri) +
                 2*(p-p0)*(q-q0)*this->c(tri) +
                 dist*dist);

    best = dist0;
    bestp = p0;
    bestq = q0;
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
        bestp = p0;
        bestq = q0;
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
        bestp = p0;
        bestq = q0;
    }
    dist = best;
    p = bestp;
    q = bestq;
    return true;
}

//=============================================================================================================

bool MNEProjectToSurface::project_to_triangle(Vector3f &rTri, const float p, const float q, const int tri)
{
    rTri = this->r1.row(tri) + p*this->r12.row(tri) + q*this->r13.row(tri);
    return true;
}
