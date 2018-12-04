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
* @brief     CircularMatrixBuffer class declaration
*
*/

#ifndef CIRCULARMATRIXBUFFER_H
#define CIRCULARMATRIXBUFFER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../utils_global.h"
#include "buffer.h"


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <typeinfo>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QPair>
#include <QSemaphore>
#include <QSharedPointer>
#include <stdio.h>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE IOBUFFER
//=============================================================================================================

namespace IOBUFFER
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;


//=============================================================================================================
/**
* Circular Matrix buffer provides a template for thread safe circular matrix buffers.
*
* @brief The circular matrix buffer
*/
template<typename _Tp>
class CircularMatrixBuffer : public Buffer
{
public:
    typedef QSharedPointer<CircularMatrixBuffer> SPtr;              /**< Shared pointer type for CircularMatrixBuffer. */
    typedef QSharedPointer<const CircularMatrixBuffer> ConstSPtr;   /**< Const shared pointer type for CircularMatrixBuffer. */

    //=========================================================================================================
    /**
    * Constructs a CircularMatrixBuffer.
    * length of buffer = uiMaxNumMatrizes*rows*cols
    *
    * @param [in] uiMaxNumMatrices  length of buffer.
    * @param [in] uiRows            Number of rows.
    * @param [in] uiCols            Number of columns.
    */
    explicit CircularMatrixBuffer(unsigned int uiMaxNumMatrices, unsigned int uiRows, unsigned int uiCols);

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

    //=========================================================================================================
    /**
    * Size of the buffer.
    */
    inline quint32 size() const;

    //=========================================================================================================
    /**
    * Rows of the stored matrices of the buffer.
    */
    inline quint32 rows() const;

    //=========================================================================================================
    /**
    * Cols of the stored matrices of the buffer.
    */
    inline quint32 cols() const;

    //=========================================================================================================
    /**
    * Pauses the buffer. Skpis any incoming matrices and only pops zero matrices.
    */
    inline void pause(bool);

    //=========================================================================================================
    /**
    * Releases the circular buffer from the acquire statement in the pop() function.
    * @param [out] bool returns true if resources were freed so that the aquire statement in the pop function can release, otherwise false.
    */
    inline bool releaseFromPop();

    //=========================================================================================================
    /**
    * Releases the circular buffer from the acquire statement in the push() function.
    * @param [out] bool returns true if resources were freed so that the aquire statement in the push function can release, otherwise false.
    */
    inline bool releaseFromPush();

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
    bool            m_bPause;
};


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

template<typename _Tp>
CircularMatrixBuffer<_Tp>::CircularMatrixBuffer(unsigned int uiMaxNumMatrices, unsigned int uiRows, unsigned int uiCols)
: Buffer(typeid(_Tp).name())
, m_uiMaxNumMatrices(uiMaxNumMatrices)
, m_uiRows(uiRows)
, m_uiCols(uiCols)
, m_uiMaxNumElements(m_uiMaxNumMatrices*m_uiRows*m_uiCols)
, m_pBuffer(new _Tp[m_uiMaxNumElements])
, m_iCurrentReadIndex(-1)
, m_iCurrentWriteIndex(-1)
, m_pFreeElements(new QSemaphore(m_uiMaxNumElements))
, m_pUsedElements(new QSemaphore(0))
, m_bPause(false)
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
    if(!m_bPause)
    {
        unsigned int t_size = pMatrix->size();
        if(t_size == m_uiRows*m_uiCols)
        {
            m_pFreeElements->acquire(t_size);
            for(unsigned int i = 0; i < t_size; ++i)
                m_pBuffer[mapIndex(m_iCurrentWriteIndex)] = pMatrix->data()[i];
            m_pUsedElements->release(t_size);
        }

        else {
            printf("Error: Matrix not appended to CircularMatrixBuffer - wrong dimensions\n");
        }
    }
}


//*************************************************************************************************************

template<typename _Tp>
inline Matrix<_Tp, Dynamic, Dynamic> CircularMatrixBuffer<_Tp>::pop()
{
    Matrix<_Tp, Dynamic, Dynamic> matrix(m_uiRows, m_uiCols);

    if(!m_bPause)
    {
        m_pUsedElements->acquire(m_uiRows*m_uiCols);
        for(quint32 i = 0; i < m_uiRows*m_uiCols; ++i)
            matrix.data()[i] = m_pBuffer[mapIndex(m_iCurrentReadIndex)];
        m_pFreeElements->release(m_uiRows*m_uiCols);
    }
    else
        matrix.setZero();

    return matrix;
}


//*************************************************************************************************************

template<typename _Tp>
inline unsigned int CircularMatrixBuffer<_Tp>::mapIndex(int& index)
{
    int AuxIndex;
    AuxIndex = ++index;
    return index = AuxIndex % m_uiMaxNumElements;

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

template<typename _Tp>
inline quint32 CircularMatrixBuffer<_Tp>::size() const
{
    return m_uiMaxNumMatrices;
}


//*************************************************************************************************************

template<typename _Tp>
inline quint32 CircularMatrixBuffer<_Tp>::rows() const
{
    return m_uiRows;
}


//*************************************************************************************************************

template<typename _Tp>
inline quint32 CircularMatrixBuffer<_Tp>::cols() const
{
    return m_uiCols;
}


//*************************************************************************************************************

template<typename _Tp>
inline void CircularMatrixBuffer<_Tp>::pause(bool bPause)
{
    m_bPause = bPause;
}


//*************************************************************************************************************

template<typename _Tp>
inline bool CircularMatrixBuffer<_Tp>::releaseFromPop()
{
   if((uint)m_pUsedElements->available() < m_uiRows*m_uiCols)
    {
        //The last matrix which is to be popped from the buffer is supposed to be a zero matrix
        unsigned int t_size = m_uiRows*m_uiCols;
        for(unsigned int i = 0; i < t_size; ++i)
            m_pBuffer[mapIndex(m_iCurrentWriteIndex)] = 0;

        //Release (create) values from m_pUsedElements so that the pop function can leave the acquire statement in the pop function
        m_pUsedElements->release(m_uiRows*m_uiCols);

        return true;
    }

    return false;
}


//*************************************************************************************************************

template<typename _Tp>
inline bool CircularMatrixBuffer<_Tp>::releaseFromPush()
{
    if((uint)m_pFreeElements->available() < m_uiRows*m_uiCols)
    {
        //The last matrix which is to be pushed to the buffer is supposed to be a zero matrix
        unsigned int t_size = m_uiRows*m_uiCols;
        for(unsigned int i = 0; i < t_size; ++i)
            m_pBuffer[mapIndex(m_iCurrentWriteIndex)] = 0;

        //Release (create) values from m_pFreeElements so that the push function can leave the acquire statement in the push function
        m_pFreeElements->release(m_uiRows*m_uiCols);

        return true;
    }

    return false;
}


//*************************************************************************************************************
//=============================================================================================================
// TYPEDEF
//=============================================================================================================

//ToDo Typedef -> warning visibility ignored -> dllexport/dllimport problem

typedef UTILSSHARED_EXPORT CircularMatrixBuffer<int>                      _int_CircularMatrixBuffer;                 /**< Defines CircularMatrixBuffer of integer type.*/
typedef UTILSSHARED_EXPORT CircularMatrixBuffer<float>                    _float_CircularMatrixBuffer;               /**< Defines CircularMatrixBuffer of float type.*/
typedef UTILSSHARED_EXPORT CircularMatrixBuffer<char>                     _char_CircularMatrixBuffer;                /**< Defines CircularMatrixBuffer of char type.*/
typedef UTILSSHARED_EXPORT CircularMatrixBuffer<double>                   _double_CircularMatrixBuffer;              /**< Defines CircularMatrixBuffer of double type.*/

typedef UTILSSHARED_EXPORT _float_CircularMatrixBuffer                   RawMatrixBuffer;                           /**< Defines RawMatrixBuffer of type _float_CircularMatrixBuffer.*/

} // NAMESPACE

#endif // CIRCULARMATRIXBUFFER_H
