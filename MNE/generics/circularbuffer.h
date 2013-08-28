//=============================================================================================================
/**
* @file     circularbuffer.h
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
* @brief     CircularBuffer class declaration
*
*/

#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "generics_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPair>
#include <QSemaphore>
#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE IOBuffer
//=============================================================================================================

namespace IOBuffer
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


//=============================================================================================================
/**
* TEMPLATE CIRCULAR BUFFER
*
* @brief The TEMPLATE CIRCULAR BUFFER provides a template for thread safe circular buffers.
*/
template<typename _Tp>
class CircularBuffer
{
public:
    typedef QSharedPointer<CircularBuffer> SPtr;              /**< Shared pointer type for CircularBuffer. */
    typedef QSharedPointer<const CircularBuffer> ConstSPtr;   /**< Const shared pointer type for CircularBuffer. */

    //=========================================================================================================
    /**
    * Constructs a CircularBuffer.
    *
    * @param [in] uiMaxNumElements length of buffer.
    */
    explicit CircularBuffer(unsigned int uiMaxNumElements);

    //=========================================================================================================
    /**
    * Destroys the CircularBuffer.
    */
    ~CircularBuffer();

    //=========================================================================================================
    /**
    * Adds a whole array at the end buffer.
    *
    * @param [in] pArray pointer to an Array which should be apend to the end.
    * @param [in] size number of elements containing the array.
    */
    inline void push(const _Tp* pArray, unsigned int size);

    //=========================================================================================================
    /**
    * Adds an element at the end of the buffer.
    *
    * @param [in] newElement pointer to an Array which should be apend to the end.
    */
    inline void push(const _Tp& newElement);

    //=========================================================================================================
    /**
    * Returns the first element (first in first out).
    *
    * @return the first element
    */
    inline _Tp pop();

    //=========================================================================================================
    /**
    * Clears the buffer.
    */
    void clear();

private:
    //=========================================================================================================
    /**
    * Returns the current circular index to the corresponding given index.
    *
    * @param [in] index which should be mapped.
    * @return the mapped index.
    */
    inline unsigned int mapIndex(int& index);
    unsigned int    m_uiMaxNumElements;     /**< Holds the maximal number of buffer elements.*/
    _Tp*            m_pBuffer;              /**< Holds the circular buffer.*/
    int             m_iCurrentReadIndex;    /**< Holds the current read index.*/
    int             m_iCurrentWriteIndex;   /**< Holds the current write index.*/
    QSemaphore*     m_pFreeElements;        /**< Holds a semaphore which acquires free elements for thread safe writing. A semaphore is a generalization of a mutex.*/
    QSemaphore*     m_pUsedElements;        /**< Holds a semaphore which acquires written semaphore for thread safe reading.*/
};


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

template<typename _Tp>
CircularBuffer<_Tp>::CircularBuffer(unsigned int uiMaxNumElements)
: m_uiMaxNumElements(uiMaxNumElements)
, m_pBuffer(new _Tp[m_uiMaxNumElements])
, m_iCurrentReadIndex(-1)
, m_iCurrentWriteIndex(-1)
, m_pFreeElements(new QSemaphore(m_uiMaxNumElements))
, m_pUsedElements(new QSemaphore(0))
{

}


//*************************************************************************************************************

template<typename _Tp>
CircularBuffer<_Tp>::~CircularBuffer()
{
    delete m_pFreeElements;
    delete m_pUsedElements;
    delete [] m_pBuffer;
}


//*************************************************************************************************************

template<typename _Tp>
inline void CircularBuffer<_Tp>::push(const _Tp* pArray, unsigned int size)
{
    m_pFreeElements->acquire(size);
    for(unsigned int i = 0; i < size; ++i)
        m_pBuffer[mapIndex(m_iCurrentWriteIndex)] = pArray[i];
    m_pUsedElements->release(size);
}


//*************************************************************************************************************

template<typename _Tp>
inline void CircularBuffer<_Tp>::push(const _Tp& newElement)
{
    m_pFreeElements->acquire();
    m_pBuffer[mapIndex(m_iCurrentWriteIndex)] = newElement;
    m_pUsedElements->release();
}


//*************************************************************************************************************

template<typename _Tp>
inline _Tp CircularBuffer<_Tp>::pop()
{
    m_pUsedElements->acquire();
    _Tp element = m_pBuffer[mapIndex(m_iCurrentReadIndex)];
    m_pFreeElements->release();

    return element;
}


//*************************************************************************************************************

template<typename _Tp>
inline unsigned int CircularBuffer<_Tp>::mapIndex(int& index)
{
    return index = ++index % m_uiMaxNumElements;
}


//*************************************************************************************************************

template<typename _Tp>
void CircularBuffer<_Tp>::clear()
{
    delete m_pFreeElements;
    m_pFreeElements = new QSemaphore(m_uiMaxNumElements);
    delete m_pUsedElements;
    m_pUsedElements = new QSemaphore(0);

    m_iCurrentReadIndex = -1;
    m_iCurrentWriteIndex = -1;
}


//*************************************************************************************************************
//=============================================================================================================
// TYPEDEF
//=============================================================================================================

//ToDo Typedef -> warning visibility ignored -> dllexport/dllimport problem

typedef GENERICSSHARED_EXPORT CircularBuffer<int>                      _int_CircularBuffer;                 /**< Defines CircularBuffer of integer type.*/
typedef GENERICSSHARED_EXPORT CircularBuffer<short>                    _short_CircularBuffer;               /**< Defines CircularBuffer of short type.*/
typedef GENERICSSHARED_EXPORT CircularBuffer<char>                     _char_CircularBuffer;                /**< Defines CircularBuffer of char type.*/
typedef GENERICSSHARED_EXPORT CircularBuffer<double>                   _double_CircularBuffer;              /**< Defines CircularBuffer of double type.*/
typedef GENERICSSHARED_EXPORT CircularBuffer< QPair<int, int> >        _int_int_pair_CircularBuffer;        /**< Defines CircularBuffer of integer Pair type.*/
typedef GENERICSSHARED_EXPORT CircularBuffer< QPair<double, double> >  _double_double_pair_CircularBuffer;  /**< Defines CircularBuffer of double Pair type.*/

typedef GENERICSSHARED_EXPORT _double_CircularBuffer                   dBuffer;             /**< Short for _double_CircularBuffer.*/
typedef GENERICSSHARED_EXPORT _int_CircularBuffer                      iBuffer;             /**< Short for _int_CircularBuffer.*/
typedef GENERICSSHARED_EXPORT _char_CircularBuffer                     cBuffer;             /**< Short for _char_CircularBuffer.*/
typedef GENERICSSHARED_EXPORT _double_CircularBuffer                   MEGBuffer;           /**< Defines MEGBuffer of type _double_CircularBuffer.*/

} // NAMESPACE

#endif // CIRCULARBUFFER_H
