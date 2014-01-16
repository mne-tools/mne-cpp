//=============================================================================================================
/**
* @file     asaelc.h
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
* @brief    AsAElc class declaration.
*
*/

#ifndef ASAELC_H
#define ASAELC_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils_global.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QVector>
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QDebug>


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
* Processes AsA .elc files which contain the electrode positions of a EEG hat.
*
* @brief Processes AsA .elc files which contain the electrode positions of a EEG hat.
*/
class UTILSSHARED_EXPORT AsAElc
{
public:
    typedef QSharedPointer<AsAElc> SPtr;            /**< Shared pointer type for AsAElc. */
    typedef QSharedPointer<const AsAElc> ConstSPtr; /**< Const shared pointer type for AsAElc. */

    //=========================================================================================================
    /**
    * Constructs a Filter object.
    */
    AsAElc();


    //=========================================================================================================
    /**
    * Gets the impulse response of a static precalculated (matlab) filter.
    * @param [in] path holds the file path of the elc file which is to be read.
    * @param [in] location3D holds the vector to which the read 3D positions are stored.
    * @param [in] location2D holds the vector to which the read 2D positions are stored.
    * @param [out] bool returns true if reading was successful, false otherwise.
    */
    bool readElcFile(QString path, QStringList &channelNames, QVector<QVector<double> > &location3D, QVector<QVector<double> > &location2D, QString &unit);

private:

};

} // NAMESPACE

#endif // ASAELC_H
