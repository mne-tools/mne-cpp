//=============================================================================================================
/**
* @file	   	typename.h
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
* @brief	ToDo Experimental - to get type
*
*/

#ifndef TYPENAME_H
#define TYPENAME_H


#define DECLARE_TYPE_NAME(x) template<> const char *typename<x>::name = #x;	/**< Macro to declare types.*/
#define GET_TYPE_NAME(x) (CTypeName<typeof(x)>::name)						/**< Macro returns the name of the type.*/

DECLARE_TYPE_NAME(int);
DECLARE_TYPE_NAME(short);
DECLARE_TYPE_NAME(char);
DECLARE_TYPE_NAME(double);

//=============================================================================================================
/**
* DECLARE CLASS CTypeName
*
* @brief The CTypeName template class provides a static const to get the type of the template by name.
*/
template <typename T> class CTypeName {
public:
    static const char *name;	/**< Holds the variable type by name.*/
};


#endif // TYPENAME_H
