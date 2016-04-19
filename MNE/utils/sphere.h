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
    Sphere( Vector3d center, double radius );

    //=========================================================================================================
    /**
    * Fits a sphere to a point cloud.
    *
    * @param[in] points     Three dimensional coordinates of the points to fit the sphere to.
    *
    * @return the fitted sphere.
    */
    static Sphere fit_sphere(const MatrixX3d& points);

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
    Vector3d& center() { return m_center; }

    //=========================================================================================================
    /**
    * The radius of the sphere.
    *
    * @return the fitted sphere.
    */
    double& radius() { return m_r; }

private:
    Vector3d m_center;      /**< Sphere's center */
    double m_r;             /**< Sphere's radius */


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

    static float fit_eval_new(const VectorXf &fitpar,
                  int   npar,
                  void  *user_data);

    static int report_func_new(int loop,
                   const VectorXf &fitpar,
                   int npar,
                   double fval);

    static float opt_rad_new(VectorXf &r0,
                  fitUserNew user);





//    static void calculate_cm_ave_dist(const MatrixX3f& rr, int np, Vector3f& cm, float *avep)
//    {
//      int k,q;
//      float ave,diff[3];

//      for (q = 0; q < 3; q++)
//        cm[q] = 0.0;

//      for (k = 0; k < np; k++)
//        for (q = 0; q < 3; q++)
//          cm[q] += rr(k,q);

//      if (np > 0) {
//        for (q = 0; q < 3; q++)
//          cm[q] = cm[q]/np;

//        for (k = 0, ave = 0.0; k < np; k++) {
//          for (q = 0; q < 3; q++)
//            diff[q] = rr(k,q) - cm[q];
//          ave += sqrt(diff[0]*diff[0]+diff[1]*diff[1]+diff[2]*diff[2]);//VEC_LEN(diff);
//        }
//        *avep = ave/np;
//      }
//      return;
//    }

//    static MatrixXf make_initial_simplex(const VectorXf& pars,
//                        float  size)
//         /*
//          * Make the initial tetrahedron
//          */
//    {
//      int npar = pars.size();
//      MatrixXf simplex = MatrixXf(npar+1,npar);
//      int k;

//      for (k = 0; k < npar+1; k++)
//        simplex.row(k) = pars;

//      for (k = 1; k < npar+1; k++)
//        simplex(k,k-1) = simplex(k,k-1) + size;
//      return simplex;
//    }


//#define VEC_DOT(x,y) ((x)[X]*(y)[X] + (x)[Y]*(y)[Y] + (x)[Z]*(y)[Z])
//#define VEC_LEN(x) sqrt(VEC_DOT(x,x))

//#define VEC_DIFF(from,to,diff) {\
//(diff)[X] = (to)[X] - (from)[X];\
//(diff)[Y] = (to)[Y] - (from)[Y];\
//(diff)[Z] = (to)[Z] - (from)[Z];\
//}

//    static float fit_eval(const Vector3f& fitpar,
//                  const MatrixX3f& rr)
//         /*
//          * Calculate the cost function value
//          * Optimize for the radius inside here
//          */
//    {
//      int np = rr.rows();
//      int   npar = fitpar.size();
//      Vector3f r0 = fitpar;
//      Vector3f diff, tmp;
//      int   k;
//      float sum,sum2,one,F;

//      for (k = 0, sum = sum2 = 0.0; k < np; k++) {
//        tmp = rr.row(k);
//        diff = tmp - r0;
//        one = diff.norm();
//        sum  += one;
//        sum2 += one*one;
//      }
//      F = sum2 - sum*sum/np;

//      fprintf(stderr,"r0 %7.1f %7.1f %7.1f R %7.1f fval %g\n",
//            1000*r0[0],1000*r0[1],1000*r0[2],1000*sum/np,F);

//      return F;
//    }


};

} // NAMESPACE

#endif // SPHERE_H
