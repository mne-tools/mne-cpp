//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file circularbuffer.h
 * @since 2022
 * @date  March 2026
 * @brief Bounded single-producer / single-consumer ring buffer used to decouple acquisition threads from processing threads.
 *
 * The template @ref UTILSLIB::CircularBuffer wraps a fixed-size
 * @c _Tp array with two @c QSemaphore counters — one tracking
 * free slots, one tracking filled slots — so @c push and @c pop
 * block (with a configurable timeout) instead of busy-waiting
 * when the buffer is full or empty. This is the building block
 * behind the @c RtClient / @c RtCmd transports in COMMUNICATIONLIB
 * and behind every plugin in MNE Scan that streams matrices
 * across thread boundaries.
 *
 * The implementation is fully header-only so consumers can
 * specialise on Eigen matrices or POD samples without forcing
 * an explicit instantiation in UTILSLIB. @c pause() lets the
 * downstream consumer drop incoming data temporarily without
 * tearing down the producer thread.
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
 * @brief Thread-safe lock-free circular (ring) buffer for producer-consumer data exchange between threads.
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
    if(m_bPause) {
        return false;
    }

    if(m_pFreeElements->tryAcquire(size, m_iTimeout)) {
        for(unsigned int i = 0; i < size; ++i) {
            m_pBuffer[mapIndex(m_iCurrentWriteIndex)] = pArray[i];
        }
        const QSemaphoreReleaser releaser(m_pUsedElements, size);
    } else {
        return false;
    }

    return true;
}

//=============================================================================================================

template<typename _Tp>
inline bool CircularBuffer<_Tp>::push(const _Tp& newElement)
{
    if(m_bPause) {
        return false;
    }

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
    if(m_bPause) {
        return false;
    }

    if(m_pUsedElements->tryAcquire(1, m_iTimeout)) {
        element = m_pBuffer[mapIndex(m_iCurrentReadIndex)];
        const QSemaphoreReleaser releaser(m_pFreeElements, 1);
    } else {
        return false;
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
