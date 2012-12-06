#ifndef MNE_MATH_GLOBAL_H
#define MNE_MATH_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(MNE_MATH_LIBRARY)
#  define MNE_MATHSHARED_EXPORT Q_DECL_EXPORT
#else
#  define MNE_MATHSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // MNE_MATH_GLOBAL_H
