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
* This class is a implementation of a queue based on a array with dynamic size.
*
* @brief Dynamic circular queue/buffer.
*/
template<typename T>
class DynamicCircularBuffer
{

public:
    typedef QSharedPointer<DynamicCircularBuffer> SPtr;            /**< Shared pointer type for DynamicCircularBuffer. */
    typedef QSharedPointer<const DynamicCircularBuffer> ConstSPtr; /**< Const shared pointer type for DynamicCircularBuffer. */

    //=========================================================================================================
    /**
    * Constructs a DynamicCircularBuffer object.
    */
    DynamicCircularBuffer();

    //=========================================================================================================
    /**
     * Constructs a DynamicCircularBuffer object.
     * @param tSize         Reserved space in the buffer.
     */
    DynamicCircularBuffer(const uint tSize);

    //=========================================================================================================
    /**
    * Copy constructs a DynamicCircularBuffer object.
    */
    DynamicCircularBuffer(const DynamicCircularBuffer& other);

    //=========================================================================================================
    /**
    * Copy operator.
    */
    DynamicCircularBuffer& operator =(const DynamicCircularBuffer& other);

    //=========================================================================================================
    /**
    * Move constructs a DynamicCircularBuffer object.
    */
    DynamicCircularBuffer(DynamicCircularBuffer&& other);

    //=========================================================================================================
    /**
    * Move operator.
    */
    DynamicCircularBuffer& operator =(const DynamicCircularBuffer&& other);

    //=========================================================================================================
    /**
     * Destructor of DynamicCircularBuffer.
     */
    ~DynamicCircularBuffer();

    //=========================================================================================================
    /**
     * Returns reference to the first element in the buffer. front of a empty buffer is undefined.
     */
    inline T& front();

    //=========================================================================================================
    /**
     * Returns copy of the first element in the buffer. front of a empty buffer is undefined.
     */
    inline T front() const;

    //=========================================================================================================
    /**
     * Overloaded [] operator for iterating over the buffer.
     */
    T& operator [](uint idx);

    //=========================================================================================================
    /**
     * Overloaded const [] operator for iterating over the buffer.
     */
    T operator [](uint idx) const;

    //=========================================================================================================
    /**
     * Adds new object to the end of the buffer.
     * @param tNewObject        Input object.
     */
    inline void push_back(const T &tNewObject);

    //=========================================================================================================
    /**
     * Pops the first object in the buffer.
     */
    inline void pop_front();

    //=========================================================================================================
    /**
     * Reserves memory for the buffer.
     * @param tSize             Size of the memory to reserve.
     */
    inline void reserve(const uint tSize);

    //=========================================================================================================
    /**
     *  Returns true if the buffer is empty.
     */
    inline bool isEmpty() const;

    //=========================================================================================================
    /**
    * Clears the buffer.
    */
    inline void clear();

    //=========================================================================================================
    /**
     * Returns the current number of objects in the buffer.
     */
    inline uint size() const;

    //=========================================================================================================
    /**
     * Returns the current capacity of the buffer.
     */
    inline uint capacity() const;

protected:

private:

    //=========================================================================================================
    /**
     * Maps tIndex to the correct correct index in the buffer. m_iHead = 0.
     * @param tIndex        Input index.
     */
    inline uint mapIndex(const uint tIndex);

    uint                m_iBufferSize;          /**< Holds the size of the buffer array.*/
    uint                m_iObjectNum;           /**< Holds the  current number of objects.*/
    T*                  m_pBuffer;              /**< Holds the circular buffer.*/
    uint                m_iHead;                /**< Holds the current read index.*/
    uint                m_iTail;                /**< Holds the current write index.*/

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
    , m_iHead(0)
    , m_iTail(0)
{

}


//*************************************************************************************************************

template<typename T>
DynamicCircularBuffer<T>::DynamicCircularBuffer(const uint tSize)
    : m_iBufferSize(tSize)
    , m_iObjectNum(0)
    , m_pBuffer(new T[m_iBufferSize])
    , m_iHead(0)
    , m_iTail(0)
{

}


//*************************************************************************************************************

template<typename T>
DynamicCircularBuffer<T>::DynamicCircularBuffer(const DynamicCircularBuffer& other)
    : m_iBufferSize(other.m_iBufferSize)
    , m_iObjectNum(other.m_iObjectNum)
    , m_pBuffer(new T[other.m_iBufferSize])
    , m_iHead(other.m_iHead)
    , m_iTail(other.m_iTail)
{
    //copy array
    for(uint i = 0; i < m_iBufferSize; i++)
    {
        m_pBuffer[i] = other.m_pBuffer[i];
    }
}


//*************************************************************************************************************

template<typename T>
DynamicCircularBuffer& DynamicCircularBuffer<T>::operator =(const DynamicCircularBuffer &other)
{
    if(this != other)
    {
        //copy variables
        m_iBufferSize = other.m_iBufferSize;
        m_iObjectNum = other.m_iObjectNum;
        m_iHead = other.m_iHead;
        m_iTail = other.m_iTail;

        //copy array
        delete[] m_pBuffer;
        m_pBuffer = new T[m_iBufferSize];
        for(uint i = 0; i < m_iBufferSize; i++)
        {
            m_pBuffer[i] = other.m_pBuffer[i];
        }
    }
    return *this;
}


//*************************************************************************************************************

template<typename T>
DynamicCircularBuffer<T>::DynamicCircularBuffer(DynamicCircularBuffer&& other)
    : m_iBufferSize(other.m_iBufferSize)
    , m_iObjectNum(other.m_iObjectNum)
    , m_iHead(other.m_iHead)
    , m_iTail(other.m_iTail)
{
    //take other's ressources
    m_pBuffer = other.m_pBuffer;

    //reset other
    other.m_iBufferSize = 0;
    other.m_iObjectNum = 0;
    other.m_iHead = 0;
    other.m_iTail = 0;
    other. m_pBuffer = nullptr;
}


//*************************************************************************************************************

template<typename T>
DynamicCircularBuffer& DynamicCircularBuffer<T>::operator =(const DynamicCircularBuffer&& other)
{
    if(this != &other)
    {
        //copy variables
        m_iBufferSize = other.m_iBufferSize;
        m_iObjectNum = other.m_iObjectNum;
        m_iHead = other.m_iHead;
        m_iTail = other.m_iTail;

        //take others resources
        delete[] m_pBuffer;
        m_pBuffer = other.m_pBuffer;

        //reset other
        other.m_pBuffer = nullptr;
        other.m_iBufferSize = 0;
        other.m_iObjectNum = 0;
        other.m_iHead = 0;
        other.m_iTail = 0;
    }
    return *this;
}


//*************************************************************************************************************

template<typename T>
DynamicCircularBuffer<T>::~DynamicCircularBuffer()
{
    delete[] m_pBuffer;
}


//*************************************************************************************************************

template<typename T>
inline T& DynamicCircularBuffer<T>::front()
{
    return m_pBuffer[m_iHead];
}


//*************************************************************************************************************

template<typename T>
inline T DynamicCircularBuffer<T>::front() const
{
    return m_pBuffer[m_iHead];
}

//*************************************************************************************************************

template<typename T>
T& DynamicCircularBuffer<T>::operator [](uint idx)
{
    return m_pBuffer[mapIndex(idx)];
}


//*************************************************************************************************************

template<typename T>
T DynamicCircularBuffer<T>::operator [](uint idx) const
{
    return m_pBuffer[mapIndex(idx)];
}


//*************************************************************************************************************

template<typename T>
inline void DynamicCircularBuffer<T>::push_back(const T &tNewObject)
{
    m_pBuffer[m_iTail] = tNewObject;
    m_iTail++;
    m_iObjectNum++;

    //Set tail to the front of the buffer if needed
    if(m_iTail == m_iBufferSize)
    {
        m_iTail = 0;
    }

    if(m_iTail == m_iHead)
    {
        reserve(2 * m_iBufferSize);
    }
}


//*************************************************************************************************************

template<typename T>
inline void DynamicCircularBuffer<T>::pop_front()
{
    m_iHead++;
    m_iObjectNum--;

    if(m_iHead == m_iBufferSize)
    {
        m_iHead = 0;
    }
}


//*************************************************************************************************************

template<typename T>
inline void DynamicCircularBuffer<T>::reserve(const uint tSize)
{
    if(tSize <= m_iBufferSize)
    {
        return;
    }

    //allocate new storage
    T *pTemp = new T[tSize];

    for(uint i = 0; i < m_iObjectNum; ++i)
    {
        pTemp[i] = m_pBuffer[mapIndex(i)];
    }

    m_iHead = 0;
    if(m_iObjectNum == 0)
    {
        m_iTail = 0;
    }
    else
    {
        m_iTail = m_iObjectNum;
    }

    delete[] m_pBuffer;
    m_pBuffer = pTemp;
    //update buffer size
    m_iBufferSize = tSize;
}


//*************************************************************************************************************

template<typename T>
inline bool DynamicCircularBuffer<T>::isEmpty() const
{
    return m_iHead == m_iTail;
}


//*************************************************************************************************************

template<typename T>
inline void DynamicCircularBuffer<T>::clear()
{
    //reset
    delete[] m_pBuffer;
    m_iTail = 0;
    m_iHead = 0;
    m_iObjectNum = 0;
    m_iBufferSize = 10;
    m_pBuffer = new T[m_iBufferSize];
}


//*************************************************************************************************************

template<typename T>
inline uint DynamicCircularBuffer<T>::size() const
{
    return m_iObjectNum;
}


//*************************************************************************************************************

template<typename T>
inline uint DynamicCircularBuffer<T>::capacity() const
{
    return m_iBufferSize;
}


//*************************************************************************************************************

template<typename T>
inline uint DynamicCircularBuffer<T>::mapIndex(const uint tIndex)
{
    return (m_iHead + tIndex) % m_iBufferSize;
}


//*************************************************************************************************************

} // namespace IOBUFFER

#endif // IOBUFFER_DYNAMICCIRCULARBUFFER_H
