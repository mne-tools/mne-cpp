#ifndef COMPUTE_SHADER_GLOBAL_H
#define COMPUTE_SHADER_GLOBAL_H

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
#  define COMPUTE_SHADERSHARED_EXPORT
#elif defined(COMPUTE_SHADER_LIBRARY)
#  define COMPUTE_SHADERSHARED_EXPORT Q_DECL_EXPORT    /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define COMPUTE_SHADERSHARED_EXPORT Q_DECL_IMPORT    /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

#endif // COMPUTE_SHADER_GLOBAL_H
