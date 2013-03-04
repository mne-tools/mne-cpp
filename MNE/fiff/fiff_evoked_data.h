//=============================================================================================================
/**
* @file     fiff_evoked_data.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    OLD FiffEvokedData class declaration.
*
*/

#ifndef FIFF_EVOKED_DATA_H
#define FIFF_EVOKED_DATA_H


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_types.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QSharedData>
#include <QSharedDataPointer>
#include <QSharedPointer>
#include <QString>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace FIFFLIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;


//=============================================================================================================
/**
* Fiff evoked data
*
* @brief evoked data
*/
class FIFFSHARED_EXPORT FiffEvokedData : public QSharedData
{
public:
    typedef QSharedPointer<FiffEvokedData> SPtr;            /**< Shared pointer type for FiffEvokedData. */
    typedef QSharedPointer<const FiffEvokedData> ConstSPtr; /**< Const shared pointer type for FiffEvokedData. */
    typedef QSharedDataPointer<FiffEvokedData> SDPtr;       /**< Shared data pointer type for FiffEvokedDataSet. */

    //=========================================================================================================
    /**
    * Constructs fiff evoked data.
    */
    FiffEvokedData();

    //=========================================================================================================
    /**
    * Copy constructor.
    *
    * @param[in] p_FiffEvokedData  Fiff evoked data which should be copied
    */
    FiffEvokedData(const FiffEvokedData &p_FiffEvokedData);

    //=========================================================================================================
    /**
    * Destroys the MNEEvokedData.
    */
    ~FiffEvokedData();

public:
    fiff_int_t  aspect_kind;    /**< Aspect identifier */
    fiff_int_t  is_smsh;        /**< ToDo... */
    fiff_int_t  nave;           /**< Number of averaged epochs. */
    fiff_int_t  first;          /**< First time sample. */
    fiff_int_t  last;           /**< Last time sample. */
    QString     comment;        /**< Comment on dataset. Can be the condition. */
    MatrixXd    times;          /**< Array of time instants in seconds. */
    MatrixXd    epochs;         /**< 2D array of shape [nChannels x nTimes]; Evoked response. */
};

} // NAMESPACE

#endif // FIFF_EVOKED_DATA_H
