#ifndef EVENTS_GLOBAL_H
#define EVENTS_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(EVENTS_LIBRARY)
#  define EVENTS_EXPORT Q_DECL_EXPORT
#else
#  define EVENTS_EXPORT Q_DECL_IMPORT
#endif

#endif // EVENTS_GLOBAL_H
