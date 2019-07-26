#ifndef APPLICATIONLOGGER_H
#define APPLICATIONLOGGER_H

#include <QtGlobal>
#include <QDebug>
#include <QTime>
#include <iostream>

namespace UTILSLIB
{
class ApplicationLogger
{
    public:
        ApplicationLogger();
        static void myCustomLogWriter(QtMsgType type, const QMessageLogContext &context, const QString &msg);
};
}
#endif // APPLICATIONLOGGER_H
