#ifndef GEOMETRYINFO_GLOBAL_H
#define GEOMETRYINFO_GLOBAL_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QtCore/qglobal.h>


//*************************************************************************************************************
//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(BUILD_MNECPP_STATIC_LIB)
#  define GEOMETRYINFOSHARED_EXPORT
#elif defined(GEOMETRYINFO_LIBRARY)
#  define GEOMETRYINFOSHARED_EXPORT Q_DECL_EXPORT    /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define GEOMETRYINFOSHARED_EXPORT Q_DECL_IMPORT    /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

#endif // GEOMETRYINFO_GLOBAL_H
