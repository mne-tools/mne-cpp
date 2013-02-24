//=============================================================================================================
/**
* @file     circularmultichannelbuffer.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the Multi Channel Circularbuffer base class.
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
