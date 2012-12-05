#ifndef FWDRT_GLOBAL_H
#define FWDRT_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(FWDRT_LIBRARY)
#  define FWDRTSHARED_EXPORT Q_DECL_EXPORT
#else
#  define FWDRTSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // FWDRT_GLOBAL_H
