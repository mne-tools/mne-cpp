//=============================================================================================================
/**
 * @file     layoutmaker.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.0
 * @date     September, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh, Gabriel Motta. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils_global.h"

#include <vector>
#include <string>
#include <fstream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>
#include <QList>
#include <QStringList>
#include <QFile>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
// TYPEDEFS
//=============================================================================================================

typedef struct {
  Eigen::MatrixXf rr;
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
     * Reads the specified ANT elc-layout file.
     * @param[in] inputPoints       The input points in 3D space.
     * @param[in, out] outputPoints     The output layout points in 2D space.
     * @param[in] names             The channel names.
     * @param[in] outFile           The outout file.
     * @param[in] do_fit            The flag whether to do a sphere fitting.
     * @param[in] prad.
     * @param[in] w.
     * @param[in] h.
     * @param[in] writeFile         The flag whether to write to file.
     * @param[in] mirrorXAxis       Mirror points at x axis.
     * @param[in] mirrorYAxis       Mirror points at y axis.
     *
     * @return true if making layout was successful, false otherwise.
     */
    static bool makeLayout(const QList<QVector<float> > &inputPoints,
                           QList<QVector<float> > &outputPoints,
                           const QStringList &names,
                           QFile &outFile,
                           bool do_fit,
                           float prad,
                           float w,
                           float h,
                           bool writeFile = false,
                           bool mirrorXAxis = false,
                           bool mirrorYAxis = false);

    //=========================================================================================================
    /**
     * Reads the specified ANT elc-layout file.
     * @param[in] inputPoints       The input points in 3D space.
     * @param[in, out] outputPoints     The output layout points in 2D space.
     * @param[in] names             The channel names.
     * @param[in] outFile           The outout file.
     * @param[in] do_fit            The flag whether to do a sphere fitting.
     * @param[in] prad.
     * @param[in] w.
     * @param[in] h.
     * @param[in] writeFile         The flag whether to write to file.
     * @param[in] mirrorXAxis       Mirror points at x axis.
     * @param[in] mirrorYAxis       Mirror points at y axis.
     *
     * @return true if making layout was successful, false otherwise.
     */
    static bool makeLayout(const std::vector<std::vector<float> > &inputPoints,
                           std::vector<std::vector<float> > &outputPoints,
                           const std::vector<std::string> &names,
                           const std::string& outFilePath,
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
};
} //NAMESPACE

#endif // LAYOUTMAKER_H
