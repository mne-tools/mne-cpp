//=============================================================================================================
/**
* @file     histogram.h
* @author   Ricky Tjen <ricky270@student.sgu.ac.id>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     March, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Ricky Tjen and Matti Hamalainen. All rights reserved.
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
* @brief    Histogram class declaration.
*/

#ifndef HISTOGRAM_H
#define HISTOGRAM_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils_global.h"
#include <math.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================
#include <QString>
#include <QVector>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//============================================================================================================
#include <Eigen/Core>


//*************************************************************************************************************
namespace UTILSLIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


class UTILSSHARED_EXPORT Histogram
{

public:
    //=========================================================================================================
    /**
    * creates a class and frequency distribution from data matrix
    *
    * @param[in]  matPresortedData       raw data matrix that needs to be analyzed
    * @param[in]  bTransposeOption       user input to turn negative to positive values
    * @param[in]  iClassCount            user input to determine the amount of classes in the histogram
    * @param[out] vecResultClassLimits   the upper limit of each individual class
    * @param[out] vecResultFrequency     the amount of data that fits in the appropriate class ranges
    * @param[in]  dGlobalMin             user input to determine the maximum value allowed in the histogram
    * @param[in]  dGlobalMax             user input to determine the minimum value allowed in the histogram
    */
    static void sort(const Eigen::MatrixXd& matPresortedData, bool bTransposeOption, int iClassAmount, QVector<double>& vecResultClassLimits, QVector<int>& vecResultFrequency, double dGlobalMin = 0.0, double dGlobalMax= 0.0);

private:
    //=========================================================================================================
    /**
    *calculates the minimum and maximum value to be used in the histogram
    *
    * @param[in] matData            raw data matrix that needs to be analyzed
    * @param[in] bTransposeOption   user input to turn negative to positive values
    *
    * @return a vector consisting of {rawMin, rawMax, localMin, localmax}
    */
    static QVector<double> findRawLocalMinMax(const Eigen::MatrixXd& matData, bool bTransposeOption);
};

}//namespace

#endif // HISTOGRAM_H

