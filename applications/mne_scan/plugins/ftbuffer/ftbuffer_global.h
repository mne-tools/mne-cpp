#ifndef FTBUFFER_GLOBAL_H
#define FTBUFFER_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(STATICLIB)
#  define FTBUFFER_EXPORT
#elif defined(FTBUFFER_LIBRARY)
#  define FTBUFFER_EXPORT Q_DECL_EXPORT
#else
#  define FTBUFFER_EXPORT Q_DECL_IMPORT
#endif

#endif // FTBUFFER_GLOBAL_H
