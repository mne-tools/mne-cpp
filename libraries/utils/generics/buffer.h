//=============================================================================================================
/**
 * @file     buffer.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  dev
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Contains the declaration of the Buffer base class.
 *
 */

#ifndef BUFFER_H
#define BUFFER_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../utils_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE IOBUFFER
//=============================================================================================================

namespace IOBUFFER
{

//=============================================================================================================
/**
 * DECLARE CLASS Buffer
 *
 * @brief The Buffer class provides a base class for buffers.
 */
class UTILSSHARED_EXPORT Buffer
{
public:
    typedef QSharedPointer<Buffer> SPtr;              /**< Shared pointer type for Buffer. */
    typedef QSharedPointer<const Buffer> ConstSPtr;   /**< Const shared pointer type for Buffer. */

    //=========================================================================================================
    /**
     * Constructs a Buffer.
     *
     * @param [in] type_id pointer to RTTI type_id of the variables of the buffer.
     */
    Buffer(const char* type_id) : m_cTypeId(type_id) {};

    //=========================================================================================================
    /**
     * Returns the type_id of the current Buffer.
     *
     * @return the RTTI type of the buffer.
     */
    inline const char* getTypeId();

private:
    const char* m_cTypeId;  /**< Holds the RTTI type_id.*/

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline const char*  Buffer::getTypeId()
{
    return m_cTypeId;
}


}//NAMESPACE


#endif // BUFFEROLD_H
