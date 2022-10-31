//=============================================================================================================
/**
 * @file     circularbuffer.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief     CircularBuffer class declaration
 *
 */

#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../utils_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPair>
#include <QSemaphore>
#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

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
     * @param[in] uiMaxNumElements length of buffer.
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
     * @param[in] pArray pointer to an Array which should be apend to the end.
     * @param[in] size number of elements containing the array.
     */
    inline bool push(const _Tp* pArray, unsigned int size);

    //=========================================================================================================
    /**
     * Adds an element at the end of the buffer.
     *
     * @param[in] newElement pointer to an Array which should be apend to the end.
     */
    inline bool push(const _Tp& newElement);

    //=========================================================================================================
    /**
     * Returns the first element (first in first out).
     *
     * @return the first element.
     */
    inline bool pop(_Tp& element);

    //=========================================================================================================
    /**
     * Clears the buffer.
     */
    void clear();

    //=========================================================================================================
    /**
     * Pauses the buffer. Skpis any incoming matrices and only pops zero matrices.
     */
    inline void pause(bool);

    //=========================================================================================================
    /**
     * Returns the number of free elements for thread safe reading.
     */
    inline int getFreeElementsRead();

    //=========================================================================================================
    /**
     * Returns the number of free elements for thread safe reading.
     */
    inline int getFreeElementsWrite();

private:
    //=========================================================================================================
    /**
     * Returns the current circular index to the corresponding given index.
     *
     * @param[in] index which should be mapped.
     * @return the mapped index.
     */
    inline unsigned int mapIndex(int& index);
    unsigned int    m_uiMaxNumElements;     /**< Holds the maximal number of buffer elements.*/
    _Tp*            m_pBuffer;              /**< Holds the circular buffer.*/
    int             m_iCurrentReadIndex;    /**< Holds the current read index.*/
    int             m_iCurrentWriteIndex;   /**< Holds the current write index.*/
    QSemaphore*     m_pFreeElements;        /**< Holds a semaphore which acquires free elements for thread safe writing. A semaphore is a generalization of a mutex.*/
    QSemaphore*     m_pUsedElements;        /**< Holds a semaphore which acquires written semaphore for thread safe reading.*/
    int             m_iTimeout;             /**< Holds the timeout value after which the acquire statement will return false.*/

    bool            m_bPause;
};

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
, m_iTimeout(1000)
, m_bPause(false)
{
}

//=============================================================================================================

template<typename _Tp>
CircularBuffer<_Tp>::~CircularBuffer()
{
    delete m_pFreeElements;
    delete m_pUsedElements;
    delete [] m_pBuffer;
}

//=============================================================================================================

template<typename _Tp>
inline bool CircularBuffer<_Tp>::push(const _Tp* pArray, unsigned int size)
{
    if(!m_bPause) {
        if(m_pFreeElements->tryAcquire(size, m_iTimeout)) {
            for(unsigned int i = 0; i < size; ++i) {
                m_pBuffer[mapIndex(m_iCurrentWriteIndex)] = pArray[i];
            }
            const QSemaphoreReleaser releaser(m_pUsedElements, size);
        } else {
            return false;
        }
    }

    return true;
}

//=============================================================================================================

template<typename _Tp>
inline bool CircularBuffer<_Tp>::push(const _Tp& newElement)
{
    if(m_pFreeElements->tryAcquire(1, m_iTimeout)) {
        m_pBuffer[mapIndex(m_iCurrentWriteIndex)] = newElement;
        const QSemaphoreReleaser releaser(m_pUsedElements, 1);
    } else {
       return false;
    }

    return true;
}

//=============================================================================================================

template<typename _Tp>
inline bool CircularBuffer<_Tp>::pop(_Tp& element)
{
    if(!m_bPause) {
        if(m_pUsedElements->tryAcquire(1, m_iTimeout)) {
            element = m_pBuffer[mapIndex(m_iCurrentReadIndex)];
            const QSemaphoreReleaser releaser(m_pFreeElements, 1);
        } else {
            return false;
        }
    }

    return true;
}

//=============================================================================================================

template<typename _Tp>
inline unsigned int CircularBuffer<_Tp>::mapIndex(int& index)
{
    int aux = index;
    return index = ++aux % m_uiMaxNumElements;
}

//=============================================================================================================

template<typename _Tp>
inline void CircularBuffer<_Tp>::clear()
{
    delete m_pFreeElements;
    m_pFreeElements = new QSemaphore(m_uiMaxNumElements);
    delete m_pUsedElements;
    m_pUsedElements = new QSemaphore(0);

    m_iCurrentReadIndex = -1;
    m_iCurrentWriteIndex = -1;
}

//=============================================================================================================

template<typename _Tp>
inline void CircularBuffer<_Tp>::pause(bool bPause)
{
    m_bPause = bPause;
}

//=============================================================================================================

template<typename _Tp>
inline int CircularBuffer<_Tp>::getFreeElementsRead()
{
    return m_pUsedElements->available();
}

//=============================================================================================================

template<typename _Tp>
inline int CircularBuffer<_Tp>::getFreeElementsWrite()
{
    return m_pFreeElements->available();
}

//=============================================================================================================
// TYPEDEF
//=============================================================================================================

typedef CircularBuffer<int>                      CircularBuffer_int;                 /**< Defines CircularBuffer of integer type.*/
typedef CircularBuffer<short>                    CircularBuffer_short;               /**< Defines CircularBuffer of short type.*/
typedef CircularBuffer<char>                     CircularBuffer_char;                /**< Defines CircularBuffer of char type.*/
typedef CircularBuffer<double>                   CircularBuffer_double;              /**< Defines CircularBuffer of double type.*/
typedef CircularBuffer< QPair<int, int> >        CircularBuffer_pair_int_int;        /**< Defines CircularBuffer of integer Pair type.*/
typedef CircularBuffer< QPair<double, double> >  CircularBuffer_pair_double_double;  /**< Defines CircularBuffer of double Pair type.*/
typedef CircularBuffer< Eigen::MatrixXd >        CircularBuffer_Matrix_double;       /**< Defines CircularBuffer of Eigen::MatrixXd type.*/
typedef CircularBuffer< Eigen::MatrixXf >        CircularBuffer_Matrix_float;        /**< Defines CircularBuffer of Eigen::MatrixXf type.*/

} // NAMESPACE

#endif // CIRCULARBUFFER_H
