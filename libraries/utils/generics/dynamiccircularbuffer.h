//=============================================================================================================
/**
* @file     dynamiccircularbuffer.h
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     September, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lars Debor and Matti Hamalainen. All rights reserved.
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
* @brief     DynamicCircularBuffer class declaration.
*
*/

#ifndef IOBUFFER_DYNAMICCIRCULARBUFFER_H
#define IOBUFFER_DYNAMICCIRCULARBUFFER_H


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
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE IOBUFFER
//=============================================================================================================

namespace IOBUFFER {


//*************************************************************************************************************
//=============================================================================================================
// IOBUFFER FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* Description of what this class is intended to do (in detail).
*
* @brief Brief description of this class.
*/
template<typename T>
class UTILSSHARED_EXPORT DynamicCircularBuffer
{

public:
    typedef QSharedPointer<DynamicCircularBuffer> SPtr;            /**< Shared pointer type for DynamicCircularBuffer. */
    typedef QSharedPointer<const DynamicCircularBuffer> ConstSPtr; /**< Const shared pointer type for DynamicCircularBuffer. */

    //=========================================================================================================
    /**
    * Constructs a DynamicCircularBuffer object.
    */
    DynamicCircularBuffer();

    DynamicCircularBuffer(const uint size);

    ~DynamicCircularBuffer();

    inline T& front();

    inline void push_back(const T &newObject);

    inline void pop_front();

    inline void reserve(const uint size);

    inline void isEmpty();

    //=========================================================================================================
    /**
    * Clears the buffer.
    */
    inline void clear();


protected:

private:

    uint            m_iBufferSize;
    uint            m_iObjectNum;
    T*              m_pBuffer;              /**< Holds the circular buffer.*/
    int             m_iCurrentReadIndex;    /**< Holds the current read index.*/
    int             m_iCurrentWriteIndex;   /**< Holds the current write index.*/

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

template<typename T>
DynamicCircularBuffer<T>::DynamicCircularBuffer()
    : m_iBufferSize(10)
    , m_iObjectNum(0)
    , m_pBuffer(new T[m_iBufferSize])
    , m_iCurrentReadIndex(0)
    , m_iCurrentWriteIndex(0)
{

}

template<typename T>
DynamicCircularBuffer<T>::DynamicCircularBuffer(const uint size)
{

}

template<typename T>
DynamicCircularBuffer<T>::~DynamicCircularBuffer()
{
    delete[] m_pBuffer;
}

template<typename T>
inline T& DynamicCircularBuffer<T>::front()
{

}

template<typename T>
inline void DynamicCircularBuffer<T>::push_back(const T &newObject)
{

}

template<typename T>
inline void DynamicCircularBuffer<T>::pop_front()
{

}

template<typename T>
inline void DynamicCircularBuffer<T>::reserve(const uint size)
{

}

template<typename T>
inline void DynamicCircularBuffer<T>::isEmpty()
{

}

template<typename T>
inline void DynamicCircularBuffer<T>::clear()
{

}


} // namespace IOBUFFER

#endif // IOBUFFER_DYNAMICCIRCULARBUFFER_H
