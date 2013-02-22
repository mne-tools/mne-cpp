//=============================================================================================================
/**
* @file		rtdtmng_global.h
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
* @brief	Contains the rtdtmng library export/import macros.
*
*/

#ifndef RTDTMNG_GLOBAL_H
#define RTDTMNG_GLOBAL_H


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/qglobal.h>


//*************************************************************************************************************
//=============================================================================================================
// PREPROCESSOR DEFINES
//=============================================================================================================

#if defined(RTDTMNG_LIBRARY)
#  define RTDTMNGSHARED_EXPORT Q_DECL_EXPORT	/**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define RTDTMNGSHARED_EXPORT Q_DECL_IMPORT	/**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

#endif // RTDTMNG_GLOBAL_H
