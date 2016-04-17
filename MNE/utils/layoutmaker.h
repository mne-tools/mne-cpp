//=============================================================================================================
/**
* @file     layoutmaker.h
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     September, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    LayoutLoader class declaration.
*
*/

#ifndef LAYOUTMAKER_H
#define LAYOUTMAKER_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils_global.h"
#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QVector>
#include <QList>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Eigen>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNELIB
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
// DEFINES
//=============================================================================================================
#ifndef FAIL
#define FAIL -1
#endif

#ifndef OK
#define OK 0
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif
#define EPS 1e-6

#ifndef M_PI
#define  M_PI   3.14159265358979323846  /* pi */
#endif


//*************************************************************************************************************
//=============================================================================================================
// TYPEDEFS
//=============================================================================================================

typedef struct {
  MatrixXf rr;
  int   np;
  int   report;
} *fitUser,fitUserRec;

//=============================================================================================================
/**
* Make layout files from given 3D points
*
* @brief Make layout files from given 3D points.
*/
class UTILSSHARED_EXPORT LayoutMaker
{
public:
    //=========================================================================================================
    /**
    * Constructs a LayoutMaker object.
    */
    LayoutMaker();

    //=========================================================================================================
    /**
    * Reads the specified ANT elc-layout file.
    * @param [in] inputPoints       The input points in 3D space.
    * @param [out] outputPoints     The output layout points in 2D space.
    * @param [in] names             The channel names.
    * @param [in] outFile           The outout file.
    * @param [in] do_fit            The flag whether to do a sphere fitting.
    * @param [in] prad
    * @param [in] w
    * @param [in] h
    * @param [in] writeFile         The flag whether to write to file.
    * @param [in] mirrorXAxis       Mirror points at x axis.
    * @param [in] mirrorYAxis       Mirror points at y axis.
    *
    * @return true if making layout was successful, false otherwise.
    */
    static bool makeLayout(const QList<QVector<double> > &inputPoints,
                           QList<QVector<double> > &outputPoints,
                           const QStringList &names,
                           QFile &outFile,
                           bool do_fit,
                           float prad,
                           float w,
                           float h,
                           bool writeFile = false,
                           bool mirrorXAxis = false,
                           bool mirrorYAxis = false);

private:
    static void sphere_coord(float x,
                      float y,
                      float z,
                      float *r,
                      float *theta,
                      float *phi);

    static int report_func(int loop,
                   const VectorXf &fitpar,
                   int npar,
                   double fval);

    static float fit_eval(const VectorXf &fitpar,
                  int   npar,
                  void  *user_data);

    static float opt_rad(VectorXf &r0,
                  fitUser user);

    static void calculate_cm_ave_dist(const MatrixXf &rr,
                               int np,
                               VectorXf &cm,
                               float &avep);

    static MatrixXf  make_initial_simplex(VectorXf &pars,
                                int    npar,
                                float  size);

    static int fit_sphere_to_points(const MatrixXf &rr,
                             int   np,
                             float simplex_size,
                             VectorXf &r0,
                             float &R);
};

} //NAMESPACE

#endif // LAYOUTMAKER_H
