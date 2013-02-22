//=============================================================================================================
/**
* @file		circularbuffer.h
* @author	Christoph Dinh <christoph.dinh@live.de>;
* @version	1.0
* @date		October, 2010
*
* @section	LICENSE
*
* Copyright (C) 2010 Christoph Dinh. All rights reserved.
*
* No part of this program may be photocopied, reproduced,
* or translated to another program language without the
* prior written consent of the author.
*
*
* @brief	Contains the declaration of the Circular Buffer class.
*
*/

#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../rtmeas_global.h"
#include "buffer.h"

#include <typeinfo>


//*************************************************************************************************************
//=============================================================================================================
// QT STL INCLUDES
//=============================================================================================================

//#include <utility> -> use instead QPair
#include <QPair>
#include <QVector>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

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


//=============================================================================================================
/**
* TEMPLATE CIRCULAR BUFFER
*
* @brief The TEMPLATE CIRCULAR BUFFER provides a template for thread safe circular buffers.
*/
template<typename _Tp>
class CircularBuffer : public Buffer
//template<typename _Tp>
//class RTMEAS_EXPORT CircularBuffer : public Buffer
{
public:
    //=========================================================================================================
    /**
    * Constructs a CircularBuffer.
    *
    * @param [in] uiMaxNumElements length of buffer.
    */
    CircularBuffer(unsigned int uiMaxNumElements);

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
    unsigned int    m_uiMaxNumElements;		/**< Holds the maximal number of buffer elements.*/
    _Tp*            m_pBuffer;				/**< Holds the circular buffer.*/
    int             m_iCurrentReadIndex;	/**< Holds the current read index.*/
    int             m_iCurrentWriteIndex;	/**< Holds the current write index.*/
    QSemaphore*     m_pFreeElements;		/**< Holds a semaphore which acquires free elements for thread safe writing. A semaphore is a generalization of a mutex.*/
    QSemaphore*     m_pUsedElements;		/**< Holds a semaphore which acquires written semaphore for thread safe reading.*/
};


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

template<typename _Tp>
CircularBuffer<_Tp>::CircularBuffer(unsigned int uiMaxNumElements)
: Buffer(typeid(_Tp).name())
, m_uiMaxNumElements(uiMaxNumElements)
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

typedef RTMEASSHARED_EXPORT CircularBuffer<int>                      _int_CircularBuffer;				/**< Defines CircularBuffer of integer type.*/
typedef RTMEASSHARED_EXPORT CircularBuffer<short>                    _short_CircularBuffer;				/**< Defines CircularBuffer of short type.*/
typedef RTMEASSHARED_EXPORT CircularBuffer<short*>                    _pShort_CircularBuffer;			/**< Defines CircularBuffer of short* type.*/
typedef RTMEASSHARED_EXPORT CircularBuffer<char>                     _char_CircularBuffer;				/**< Defines CircularBuffer of char type.*/
typedef RTMEASSHARED_EXPORT CircularBuffer<double>                   _double_CircularBuffer;			/**< Defines CircularBuffer of double type.*/
typedef RTMEASSHARED_EXPORT CircularBuffer< QPair<int, int> >        _int_int_pair_CircularBuffer;		/**< Defines CircularBuffer of integer Pair type.*/
typedef RTMEASSHARED_EXPORT CircularBuffer< QPair<double, double> >  _double_double_pair_CircularBuffer;	/**< Defines CircularBuffer of double Pair type.*/

typedef RTMEASSHARED_EXPORT _double_CircularBuffer                   dBuffer;				/**< Short for _double_CircularBuffer.*/
typedef RTMEASSHARED_EXPORT _int_CircularBuffer                      iBuffer;				/**< Short for _int_CircularBuffer.*/
typedef RTMEASSHARED_EXPORT _char_CircularBuffer                     cBuffer;				/**< Short for _char_CircularBuffer.*/
typedef RTMEASSHARED_EXPORT _pShort_CircularBuffer                   pShortBuffer;		/**< Short for _pShort_CircularBuffer.*/
typedef RTMEASSHARED_EXPORT _double_CircularBuffer                   MEGBuffer;			/**< Defines MEGBuffer of type _double_CircularBuffer.*/
typedef RTMEASSHARED_EXPORT _double_CircularBuffer                   ECGBuffer;			/**< Defines ECGBuffer of type _double_CircularBuffer.*/
typedef RTMEASSHARED_EXPORT _double_CircularBuffer                   DummyBuffer;			/**< Defines DummyBuffer of type _double_CircularBuffer.*/
typedef RTMEASSHARED_EXPORT _double_CircularBuffer                   WaveletBuffer;		/**< Defines WaveletBuffer of type _double_CircularBuffer.*/
typedef RTMEASSHARED_EXPORT _double_CircularBuffer                   FilterBuffer;		/**< Defines FilterBuffer of type _double_CircularBuffer.*/
typedef RTMEASSHARED_EXPORT _double_CircularBuffer                   GaborParticleBuffer;	/**< Defines GaborParticleBuffer of type _double_CircularBuffer.*/

} // NAMESPACE

#endif // CIRCULARBUFFER_H
