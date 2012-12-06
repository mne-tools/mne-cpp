#ifndef FWD_GLOBAL_H
#define FWD_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(FWD_LIBRARY)
#  define FWDSHARED_EXPORT Q_DECL_EXPORT
#else
#  define FWDSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // FWD_GLOBAL_H
