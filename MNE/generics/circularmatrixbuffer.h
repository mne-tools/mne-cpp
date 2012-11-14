//=============================================================================================================
/**
* @file     circularmatrixbuffer.h
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
* @brief    Contains the CircularMatrixBuffer class declaration
*
*/

#ifndef CIRCULARMATRIXBUFFER_H
#define CIRCULARMATRIXBUFFER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "generics_global.h"


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../include/3rdParty/Eigen/Core"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPair>
#include <QSemaphore>


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

using namespace Eigen;


//=============================================================================================================
/**
* TEMPLATE CIRCULAR BUFFER
*
* @brief The TEMPLATE CIRCULAR BUFFER provides a template for thread safe circular buffers.
*/
template<typename _Tp>
class CircularMatrixBuffer
{
public:
    //=========================================================================================================
    /**
    * Constructs a CircularMatrixBuffer.
    * length of buffer = uiMaxNumMatrizes*rows*cols
    *
    * @param [in] uiMaxNumMatrices  length of buffer.
    */
    CircularMatrixBuffer(unsigned int uiMaxNumMatrices, unsigned int uiRows, unsigned int uiCols);

    //=========================================================================================================
    /**
    * Destroys the CircularBuffer.
    */
    ~CircularMatrixBuffer();

    //=========================================================================================================
    /**
    * Adds a whole matrix at the end buffer.
    *
    * @param [in] pMatrix pointer to a Matrix which should be apend to the end.
    * @param [in] size number of elements containing the array.
    */
    inline void push(const Matrix<_Tp, Dynamic, Dynamic>* pMatrix);

    //=========================================================================================================
    /**
    * Returns the first matrix (first in first out).
    *
    * @return the first matrix
    */
    inline Matrix<_Tp, Dynamic, Dynamic> pop();

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
    unsigned int    m_uiMaxNumMatrices;         /**< Holds the maximal number of matrices.*/
    unsigned int    m_uiRows;                   /**< Holds the number rows.*/
    unsigned int    m_uiCols;                   /**< Holds the number cols.*/
    unsigned int    m_uiMaxNumElements;         /**< Holds the maximal number of buffer elements.*/
    _Tp*            m_pBuffer;                  /**< Holds the circular buffer.*/
    int             m_iCurrentReadIndex;        /**< Holds the current read index.*/
    int             m_iCurrentWriteIndex;       /**< Holds the current write index.*/
    QSemaphore*     m_pFreeElements;            /**< Holds a semaphore which acquires free elements for thread safe writing. A semaphore is a generalization of a mutex.*/
    QSemaphore*     m_pUsedElements;            /**< Holds a semaphore which acquires written semaphore for thread safe reading.*/
};


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

template<typename _Tp>
CircularMatrixBuffer<_Tp>::CircularMatrixBuffer(unsigned int uiMaxNumMatrices, unsigned int uiRows, unsigned int uiCols)
: m_uiMaxNumMatrices(uiMaxNumMatrices)
, m_uiRows(uiRows)
, m_uiCols(uiCols)
, m_uiMaxNumElements(m_uiMaxNumMatrices*m_uiRows*m_uiCols)
, m_pBuffer(new _Tp[m_uiMaxNumElements])
, m_iCurrentReadIndex(-1)
, m_iCurrentWriteIndex(-1)
, m_pFreeElements(new QSemaphore(m_uiMaxNumElements))
, m_pUsedElements(new QSemaphore(0))
{

}


//*************************************************************************************************************

template<typename _Tp>
CircularMatrixBuffer<_Tp>::~CircularMatrixBuffer()
{
    delete m_pFreeElements;
    delete m_pUsedElements;
    delete [] m_pBuffer;
}


//*************************************************************************************************************

template<typename _Tp>
inline void CircularMatrixBuffer<_Tp>::push(const Matrix<_Tp, Dynamic, Dynamic>* pMatrix)
{
    unsigned int t_size = pMatrix->size();
    if(t_size == m_uiRows*m_uiCols)
    {
        m_pFreeElements->acquire(t_size);
        for(unsigned int i = 0; i < t_size; ++i)
            m_pBuffer[mapIndex(m_iCurrentWriteIndex)] = pMatrix->data()[i];
        m_pUsedElements->release(t_size);
    }
    else
        printf("Error: Matrix not appended to CircularMatrixBuffer - wrong dimensions\n");
}


//*************************************************************************************************************

template<typename _Tp>
inline Matrix<_Tp, Dynamic, Dynamic> CircularMatrixBuffer<_Tp>::pop()
{
    m_pUsedElements->acquire(m_uiRows*m_uiCols);
    Matrix<_Tp, Dynamic, Dynamic> matrix(m_uiRows, m_uiCols);
    for(quint32 i = 0; i < m_uiRows*m_uiCols; ++i)
        matrix.data()[i] = m_pBuffer[mapIndex(m_iCurrentReadIndex)];
    m_pFreeElements->release(m_uiRows*m_uiCols);

    return matrix;
}


//*************************************************************************************************************

template<typename _Tp>
inline unsigned int CircularMatrixBuffer<_Tp>::mapIndex(int& index)
{
    return index = ++index % m_uiMaxNumElements;
}


//*************************************************************************************************************

template<typename _Tp>
void CircularMatrixBuffer<_Tp>::clear()
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

typedef GENERICSSHARED_EXPORT CircularMatrixBuffer<int>                      _int_CircularMatrixBuffer;                 /**< Defines CircularMatrixBuffer of integer type.*/
typedef GENERICSSHARED_EXPORT CircularMatrixBuffer<float>                    _float_CircularMatrixBuffer;               /**< Defines CircularMatrixBuffer of float type.*/
typedef GENERICSSHARED_EXPORT CircularMatrixBuffer<char>                     _char_CircularMatrixBuffer;                /**< Defines CircularMatrixBuffer of char type.*/
typedef GENERICSSHARED_EXPORT CircularMatrixBuffer<double>                   _double_CircularMatrixBuffer;              /**< Defines CircularMatrixBuffer of double type.*/

typedef GENERICSSHARED_EXPORT _float_CircularMatrixBuffer                   RawMatrixBuffer;                           /**< Defines RawMatrixBuffer of type _float_CircularMatrixBuffer.*/

} // NAMESPACE

#endif // CIRCULARMATRIXBUFFER_H
