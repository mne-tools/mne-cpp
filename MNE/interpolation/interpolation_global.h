#ifndef INTERPOLATION_GLOBAL_H
#define INTERPOLATION_GLOBAL_H

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
#  define INTERPOLATIONSHARED_EXPORT
#elif defined(INTERPOLATION_LIBRARY)
#  define INTERPOLATIONSHARED_EXPORT Q_DECL_EXPORT    /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define INTERPOLATIONSHARED_EXPORT Q_DECL_IMPORT    /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

#endif // INTERPOLATION_GLOBAL_H
