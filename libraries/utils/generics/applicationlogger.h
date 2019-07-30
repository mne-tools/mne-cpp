#ifndef APPLICATIONLOGGER_H
#define APPLICATIONLOGGER_H

#include "../utils_global.h"

#include <QSharedPointer>

class QMutex;

namespace UTILSLIB
{
class UTILSSHARED_EXPORT ApplicationLogger
{
    public:
        ApplicationLogger();
        static void myCustomLogWriter(QtMsgType type, const QMessageLogContext &context, const QString &msg);

};
}
#endif // APPLICATIONLOGGER_H
