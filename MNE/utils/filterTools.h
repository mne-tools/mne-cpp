//=============================================================================================================
/**
* @file     filtertools.h
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    FilterTools class declaration.
*
*/

#ifndef FILTERTOOLS_H
#define FILTERTOOLS_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils_global.h"
#include <qmath.h>

//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QVector>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


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


//=============================================================================================================
/**
* Basic filter creation and operations (HP, TP, BP)
*
* @brief Basic filter creation and operations (HP, TP, BP)
*/
class UTILSSHARED_EXPORT FilterTools
{
public:
    typedef QSharedPointer<FilterTools> SPtr;            /**< Shared pointer type for FilterTools. */
    typedef QSharedPointer<const FilterTools> ConstSPtr; /**< Const shared pointer type for FilterTools. */

    //=========================================================================================================
    /**
    * Constructs a Filter object.
    */
    FilterTools();


    //=========================================================================================================
    /**
    * Gets the impulse response of a static precalculated (matlab) filter.
    * @param [in] type specifies the type (high-, low- , band-pass) of the filter which is to be designed. Possible values: 'HP', 'LP', 'BP'.
    * @param [in] numberOfCoefficients number of coefficients used for the filter.
    * @param [in] samplingRate holds the sampling frequency for the filter. Possible values: 128, 256, 512, 1024, 2048.
    * @param [in] cutOffFreq holds the cut off frequency for the filter. Max possible value: samplingRate/2.
    * @param [in] impulseResponse holds the created coefficients (impulse response) of the filter.
    */
    void getStaticFilter(QString type, qint32 numberOfCoefficients, qint32 samplingRate, qint32 cutOffFreq, QVector<float> &impulseResponse);


    //=========================================================================================================
    /**
    * Creates a Filter.
    * @param [in] type specifies the type (high-, low- , band-pass) of the filter which is to be designed. Possible values: 'HP', 'LP', 'BP'.
    * @param [in] numberOfCoefficients number of coefficients used for the filter.
    * @param [in] normalizedCutOffFreq holds the cut off frequency for the filter. Range [0 1] whre 1 (pi) corresponds to f_max.
    * @param [in] impulseResponse holds the created coefficients (impulse response) of the filter.
    */
    void createDynamicFilter(QString type, qint32 numberOfCoefficients, float normalizedCutOffFreq, QVector<float> &impulseResponse);


    //=========================================================================================================
    /**
    * Convolves a given data set with a given filter impulse response.
    * @param [in] in holds the data which is to be filtered.
    * @param [in] kernel holds the impulse response of the filter which is to be used during the convolution.
    * @param [out] QVector<float> holds the result of the convolution.
    */
    QVector<float> convolve(QVector<float> &in, QVector<float> &kernel);

private:
    /**
    * Creates a kaiser window. Regular Modified Cylindrical Bessel Function (Bessel I).
    * @param [in] window
    * @param [in] size
    * @param [in] alpha
    */
    void KBDWindow(QVector<float> &window, int size, float alpha);

    //=========================================================================================================
    /**
    * Calculates Bssel function.
    * @param [in] x
    */
    float BesselI0(float x);
};

} // NAMESPACE

#endif // FILTERTOOLS_H
