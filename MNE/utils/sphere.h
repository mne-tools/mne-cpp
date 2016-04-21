//=============================================================================================================
/**
* @file     mnemath.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     April, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    MNEMath class declaration.
*
*/

#ifndef SPHERE_H
#define SPHERE_H

//ToDo move this to the new MNE math library

//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include "utils_global.h"

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================




//*************************************************************************************************************
//=============================================================================================================
// TYPEDEFS
//=============================================================================================================

typedef struct {
  MatrixXf rr;
  bool   report;
} *fitUserNew,fitUserRecNew;


//=============================================================================================================
/**
* Sphere descritpion
*
* @brief Describes a 3D sphere object
*/
class UTILSSHARED_EXPORT Sphere
{
public:

    //=========================================================================================================
    /**
    * Constructs the Sphere
    *
    * @param[in] center     The sphere's center
    * @param[in] radius     The sphere's radius
    */
    Sphere( const Vector3f& center, float radius );

    //=========================================================================================================
    /**
    * Fits a sphere to a point cloud.
    *
    * @param[in] points     Three dimensional coordinates of the points to fit the sphere to.
    *
    * @return the fitted sphere.
    */
    static Sphere fit_sphere(const MatrixX3f& points);

    //=========================================================================================================
    /**
    * Fits a sphere to a point cloud.
    *
    * @param[in] points         Three dimensional coordinates of the points to fit the sphere to.
    * @param[in] simplex_size   The simplex size
    *
    * @return the fitted sphere.
    */
    static Sphere fit_sphere_simplex(const MatrixX3f& points, double simplex_size = 2e-2);

    //=========================================================================================================
    /**
    * The radius of the sphere.
    *
    * @return the fitted sphere.
    */
    Vector3f& center() { return m_center; }

    //=========================================================================================================
    /**
    * The radius of the sphere.
    *
    * @return the fitted sphere.
    */
    float& radius() { return m_r; }

private:
    Vector3f m_center;      /**< Sphere's center */
    float m_r;             /**< Sphere's radius */


    //ToDo Replace LayoutMaker fit_sphere_to_points
    static bool fit_sphere_to_points_new (  const MatrixXf &rr,
                                            float simplex_size,
                                            VectorXf &r0,
                                            float &R );

    static void calculate_cm_ave_dist_new(  const MatrixXf &rr,
                                            VectorXf &cm,
                                            float &avep );

    static MatrixXf make_initial_simplex_new(   const VectorXf &pars,
                                                float  size );

    static float fit_eval_new(  const VectorXf &fitpar,
                                int   npar,
                                const void  *user_data);

    static int report_func_new(int loop,
                   const VectorXf &fitpar,
                   int npar,
                   double fval);

    static float opt_rad_new(   const VectorXf &r0,
                                const fitUserNew user);

};

} // NAMESPACE

#endif // SPHERE_H
