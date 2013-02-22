//=============================================================================================================
/**
* @file		circularmultichannelbuffer.h
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
* @brief	Contains the declaration of the Multi Channel Circularbuffer base class.
*
*/

#ifndef CIRCULARMULTICHANNELBUFFER_H
#define CIRCULARMULTICHANNELBUFFER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "circularbuffer.h"


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <QVector>


//*************************************************************************************************************
//=============================================================================================================
// QT STL INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


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
class CircularMultiChannelBuffer : public Buffer
//template<typename _Tp>
//class RTMEAS_EXPORT CircularMultiChannelBuffer : public Buffer
{
public:
    //=========================================================================================================
    /**
    * Constructs a CircularMultiChannelBuffer.
    *
    * @param [in] uiChannel selected channel.
    * @param [in] uiMaxNumElements length of buffer.
    */
    CircularMultiChannelBuffer(unsigned int uiNumChannels, unsigned int uiMaxNumElements);

    //=========================================================================================================
    /**
    * Destroys the CircularMultiChannelBuffer.
    */
    ~CircularMultiChannelBuffer();

    //=========================================================================================================
    /**
    * Initializes the buffer.
    */
    void init();

    //=========================================================================================================
    /**
    * Adds a whole array at the end buffer.
    *
    * @param [in] uiChannel selected channel.
    * @param [in] pArray pointer to an Array which should be append to the end.
    * @param [in] size number of elements containing the array.
    */
    inline void push(unsigned int uiChannel, const _Tp* pArray, unsigned int size);

    //=========================================================================================================
    /**
    * Adds an element at the end of the buffer.
    *
    * @param [in] uiChannel selected channel.
    * @param [in] newElement pointer to an Array which should be append to the end.
    */
    inline void push(unsigned int uiChannel, const _Tp& newElement);

    //=========================================================================================================
    /**
    * Adds an element at the end of the buffer.
    *
    * @param [in] newElements vector which should be added to the channels.
    */
    inline void push(const QVector<_Tp>& newElements);

    //=========================================================================================================
    /**
    * Returns the first element (first in first out).
    *
    * @return the first element
    */
    inline QVector<_Tp> pop();

    //=========================================================================================================
    /**
    * Returns the first element (first in first out) of a specific channel.
    *
    * @param [in] uiChannel selected channel.
    * @return the first element of the specific channel
    */
    inline _Tp pop(unsigned int uiChannel);

    //=========================================================================================================
    /**
    * Returns the number of channels.
    */
    inline int numChannels();


    //=========================================================================================================
    /**
    * Clears the buffer.
    */
    void clear();

private:
    QVector< CircularBuffer<_Tp>* >* m_qVecBuffers;

    unsigned int    m_uiMaxNumElements;		/**< Holds the maximal number of buffer elements.*/


};


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

template<typename _Tp>
CircularMultiChannelBuffer<_Tp>::CircularMultiChannelBuffer(unsigned int uiNumChannels, unsigned int uiMaxNumElements)
: Buffer(typeid(_Tp).name())
, m_qVecBuffers(new QVector< CircularBuffer<_Tp>* >(uiNumChannels))
, m_uiMaxNumElements(uiMaxNumElements)
{
    init();
}


//*************************************************************************************************************

template<typename _Tp>
CircularMultiChannelBuffer<_Tp>::~CircularMultiChannelBuffer()
{
    clear();
    delete m_qVecBuffers;
}


//*************************************************************************************************************

template<typename _Tp>
void CircularMultiChannelBuffer<_Tp>::init()
{
//     clear();
//     for (int i = 0; i < uiNumChannels; i++)
//         m_qVecBuffers.push_back( new  CircularBuffer<_Tp>(m_uiMaxNumElements) );


    for (typename  QVector< CircularBuffer<_Tp>* >::iterator i = m_qVecBuffers->begin(); i != m_qVecBuffers->end(); ++i) {
        *i = new  CircularBuffer<_Tp>(m_uiMaxNumElements);
    }

}


//*************************************************************************************************************

template<typename _Tp>
inline int CircularMultiChannelBuffer<_Tp>::numChannels()
{
    return m_qVecBuffers->size();
}


//*************************************************************************************************************

template<typename _Tp>
inline void CircularMultiChannelBuffer<_Tp>::push(unsigned int uiChannel, const _Tp* pArray, unsigned int size)
{
    m_qVecBuffers[uiChannel]->push(pArray, size);
}


//*************************************************************************************************************

template<typename _Tp>
inline void CircularMultiChannelBuffer<_Tp>::push(unsigned int uiChannel, const _Tp& newElement)
{
    (*m_qVecBuffers)[uiChannel]->push(newElement);
}


//*************************************************************************************************************

template<typename _Tp>
inline void CircularMultiChannelBuffer<_Tp>::push(const QVector<_Tp>& newElements)
{
    for(int i = 0; i < newElements.size(); ++i)
        (*m_qVecBuffers)[i]->push(newElements[i]);
}


//*************************************************************************************************************

template<typename _Tp>
inline QVector<_Tp> CircularMultiChannelBuffer<_Tp>::pop()
{
    QVector<_Tp> elements;

    for (typename QVector< CircularBuffer<_Tp>* >::iterator i = m_qVecBuffers->begin(); i != m_qVecBuffers->end(); ++i) {
        elements.push_back((*i)->pop());
    }

    return elements;
}


//*************************************************************************************************************

template<typename _Tp>
inline _Tp CircularMultiChannelBuffer<_Tp>::pop(unsigned int uiNumChannels)
{
        return (*m_qVecBuffers)[uiNumChannels]->pop();
}


//*************************************************************************************************************

template<typename _Tp>
void CircularMultiChannelBuffer<_Tp>::clear()
{
    for (typename  QVector< CircularBuffer<_Tp>* >::iterator i = m_qVecBuffers->begin(); i != m_qVecBuffers->end(); ++i) {
        delete i;
        *i = NULL;
    }
}


//*************************************************************************************************************
//=============================================================================================================
// TYPEDEF
//=============================================================================================================

//ToDo Typedef -> warning visibility ignored -> dllexport/dllimport problem

typedef RTMEASSHARED_EXPORT CircularMultiChannelBuffer<int>                      _int_CircularMultiChannelBuffer;				/**< Defines CircularBuffer of integer type.*/
typedef RTMEASSHARED_EXPORT CircularMultiChannelBuffer<short>                    _short_CircularMultiChannelBuffer;				/**< Defines CircularBuffer of short type.*/
typedef RTMEASSHARED_EXPORT CircularMultiChannelBuffer<short*>                    _pShort_CircularMultiChannelBuffer;			/**< Defines CircularBuffer of short* type.*/
typedef RTMEASSHARED_EXPORT CircularMultiChannelBuffer<char>                     _char_CircularMultiChannelBuffer;				/**< Defines CircularBuffer of char type.*/
typedef RTMEASSHARED_EXPORT CircularMultiChannelBuffer<double>                   _double_CircularMultiChannelBuffer;			/**< Defines CircularBuffer of double type.*/
typedef RTMEASSHARED_EXPORT CircularMultiChannelBuffer< QPair<int, int> >        _int_int_pair_CircularMultiChannelBuffer;		/**< Defines CircularBuffer of integer Pair type.*/
typedef RTMEASSHARED_EXPORT CircularMultiChannelBuffer< QPair<double, double> >  _double_double_pair_CircularMultiChannelBuffer;	/**< Defines CircularBuffer of double Pair type.*/

} // NAMESPACE

#endif // CIRCULARMULTICHANNELBUFFER_H
