//=============================================================================================================
/**
* @file		buffer.h
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
* @brief	Contains the declaration of the Buffer base class.
*
*/

#ifndef BUFFER_H
#define BUFFER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../rtmeas_global.h"


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE IOBuffer
//=============================================================================================================

namespace IOBuffer
{

//=============================================================================================================
/**
* DECLARE CLASS Buffer
*
* @brief The Buffer class provides a base class for buffers.
*/
class RTMEASSHARED_EXPORT Buffer
{
public:

    //=========================================================================================================
    /**
    * Constructs a Buffer.
    *
    * @param [in] type_id pointer to RTTI type_id of the variables of the buffer.
    */
    Buffer(const char* type_id) : m_cTypeId(type_id) {};
    //=========================================================================================================
    /**
    * Returns the type_id of the current Buffer.
    *
    * @return the RTTI type of the buffer.
    */
    inline const char* getTypeId();

private:
    const char* m_cTypeId;		/**< Holds the RTTI type_id.*/

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline const char*  Buffer::getTypeId()
{
	return m_cTypeId;
}


}//NAMESPACE


#endif // BUFFER_H
