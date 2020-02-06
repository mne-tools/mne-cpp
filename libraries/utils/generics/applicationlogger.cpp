//=============================================================================================================
/**
 * @file     applicationlogger.h
 * @author   Mainak Jas <mjas@mgh.harvard.edu>;
 *           Ruben Dörfel <rdorfel@mgh.harvard.edu>
 * @version  dev
 * @date     July, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Mainak Jas, Ruben Dörfel. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Contains the declaration of the applicationlogger class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include "applicationlogger.h"

//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QtGlobal>
#include <QDebug>
#include <QTime>

#include <QMutex>
#include <QMutexLocker>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


using namespace UTILSLIB;

//*************************************************************************************************************
//=============================================================================================================
// Definitions
//=============================================================================================================

#define COLOR_DEBUG "\033[32;1m"
#define COLOR_WARN "\033[33;1m"
#define COLOR_CRITICAL "\033[31;1m"
#define COLOR_FATAL "\033[33;1m"
#define COLOR_RESET "\033[0m"

#define LOG_WRITE(OUTPUT, COLOR, LEVEL, MSG) OUTPUT << COLOR << \
" " LEVEL " " << MSG << COLOR_RESET << "\n"

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================


ApplicationLogger::ApplicationLogger()
{

}

void ApplicationLogger::myCustomLogWriter(QtMsgType type,
                                          const QMessageLogContext &context,
                                          const QString &msg) {

    static QMutex mutex;
    QMutexLocker locker(&mutex);

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
