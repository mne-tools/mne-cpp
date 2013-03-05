//=============================================================================================================
/**
* @file     ioutils.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     March, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    IOUtils class declaration
*
*/

#ifndef IOUTILS_H
#define IOUTILS_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FSLIB
//=============================================================================================================

namespace UTILSLIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* IO utilitie routines
*
* @brief IO utilitie routines
*/
class UTILSSHARED_EXPORT IOUtils
{
public:
    typedef QSharedPointer<IOUtils> SPtr;            /**< Shared pointer type for IOUtils class. */
    typedef QSharedPointer<const IOUtils> ConstSPtr; /**< Const shared pointer type for IOUtils class. */

    //=========================================================================================================
    /**
    * Destroys the IOUtils class.
    */
    ~IOUtils(){};

    //=========================================================================================================
    /**
    * mne_fread3(fid) -> ToDO put this into a sparate mneHelpers class
    *
    * Reads a 3-byte integer out of a stream
    *
    * @param[in] p_qStream  Stream to read from
    *
    * @return the read 3-byte integer
    */
    static qint32 fread3(QDataStream &p_qStream);

    //=========================================================================================================
    /**
    * swap short
    *
    * @param[in] source     short to swap
    *
    * @return swapped short
    */
    static qint16 swap_short (qint16 source);

    //=========================================================================================================
    /**
    * swap integer
    *
    * @param[in] source     integer to swap
    *
    * @return swapped integer
    */
    static qint32 swap_int (qint32 source);

    //=========================================================================================================
    /**
    * swap integer
    *
    * @param[in, out] source     integer to swap
    *
    * @return swapped integer
    */
    static void swap_intp (qint32 *source);

    //=========================================================================================================
    /**
    * swap long
    *
    * @param[in] source     long to swap
    *
    * @return swapped long
    */
    static qint64 swap_long (qint64 source);

    //=========================================================================================================
    /**
    * swap long
    *
    * @param[in, out] source     long to swap
    *
    * @return swapped long
    */
    static void swap_longp (qint64 *source);

    //=========================================================================================================
    /**
    * swap float
    *
    * @param[in, out] source     float to swap
    *
    * @return swapped float
    */
    static void swap_floatp (float *source);

    //=========================================================================================================
    /**
    * swap double
    *
    * @param[in, out] source     double to swap
    *
    * @return swapped double
    */
    static void swap_doublep(double *source);
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // NAMESPACE

#endif // IOUTILS_H

