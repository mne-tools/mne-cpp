#include "applicationlogger.h"
using namespace UTILSLIB;

#define COLOR_DEBUG "\033[32;1m"
#define COLOR_WARN "\033[33;1m"
#define COLOR_CRITICAL "\033[31;1m"
#define COLOR_FATAL "\033[33;1m"
#define COLOR_RESET "\033[0m"

#define LOG_WRITE(OUTPUT, COLOR, LEVEL, MSG) OUTPUT << COLOR << \
" " LEVEL " " << MSG << COLOR_RESET << "\n"

ApplicationLogger::ApplicationLogger()
{

}

void ApplicationLogger::myCustomLogWriter(QtMsgType type,
                                          const QMessageLogContext &context, const QString &msg) {
    switch (type) {
        case QtWarningMsg:
            LOG_WRITE(std::cout, COLOR_WARN, "WARN", msg.toStdString());
            break;
        case QtCriticalMsg:
            LOG_WRITE(std::cout, COLOR_CRITICAL, "CRIT", msg.toStdString());
            break;
        case QtFatalMsg:
            LOG_WRITE(std::cout, COLOR_FATAL, "FATAL", msg.toStdString());
            break;
        case QtDebugMsg:
            LOG_WRITE(std::cout, COLOR_DEBUG, "DEBUG", msg.toStdString());
            break;
    }
}
