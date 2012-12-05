#ifndef INVRT_GLOBAL_H
#define INVRT_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(INVRT_LIBRARY)
#  define INVRTSHARED_EXPORT Q_DECL_EXPORT
#else
#  define INVRTSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // INVRT_GLOBAL_H
