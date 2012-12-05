#ifndef FS_GLOBAL_H
#define FS_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(FS_LIBRARY)
#  define FSSHARED_EXPORT Q_DECL_EXPORT
#else
#  define FSSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // FS_GLOBAL_H
