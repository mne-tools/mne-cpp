#ifndef MNEMATH_GLOBAL_H
#define MNEMATH_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(MNEMATH_LIBRARY)
#  define MNEMATHSHARED_EXPORT Q_DECL_EXPORT
#else
#  define MNEMATHSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // MNEMATH_GLOBAL_H
