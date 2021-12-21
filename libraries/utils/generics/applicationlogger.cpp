//=============================================================================================================
/**
 * @file     applicationlogger.h
 * @author   Mainak Jas <mjas@mgh.harvard.edu>;
 *           Ruben Dörfel <rdorfel@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Mainak Jas, Ruben Dörfel. All rights reserved.
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
 * @brief    Contains the declaration of the ApplicationLogger class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include "applicationlogger.h"
#include <stdio.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtGlobal>
#include <QDebug>
#include <QTime>
#include <QMutexLocker>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace std;

//=============================================================================================================
// Definitions
//=============================================================================================================

#ifdef WIN32
    #include <wchar.h>
    #include <windows.h>
    #define COLOR_INFO          SetConsoleTextAttribute(hOut, 0x02)     //green
    #define COLOR_DEBUG         SetConsoleTextAttribute(hOut, 0x02)     //green
    #define COLOR_WARN          SetConsoleTextAttribute(hOut, 0x0E)     //yellow
    #define COLOR_FATAL         SetConsoleTextAttribute(hOut, 0x0E)     //yellow
    #define COLOR_CRITICAL      SetConsoleTextAttribute(hOut, 0x04)     //red
    #define COLOR_RESET         SetConsoleTextAttribute(hOut, 7)        //reset
    #define LOG_WRITE(OUTPUT, COLOR, LEVEL, MSG) COLOR; OUTPUT << LEVEL;COLOR_RESET; OUTPUT<< MSG << std::endl

#else
    #define COLOR_INFO          "\033[32;1m"        //green
    #define COLOR_DEBUG         "\033[32;1m"        //green
    #define COLOR_WARN          "\033[33;1m"        //yellow
    #define COLOR_CRITICAL      "\033[31;1m"        //red
    #define COLOR_FATAL         "\033[33;1m"        //yellow
    #define COLOR_RESET         "\033[0m"           //reset

    #define LOG_WRITE(OUTPUT, COLOR, LEVEL, MSG) OUTPUT << COLOR << LEVEL << COLOR_RESET << MSG << "\n"
#endif

std::mutex ApplicationLogger::m_mutex;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void ApplicationLogger::customLogWriter(QtMsgType type,
                                        const QMessageLogContext &context,
                                        const QString &msg)
{
    Q_UNUSED(context)

    QString sDate = QString("] ");
    // Comment in following line if you want to display the date and time of the message
    //sDate = QString(" %1] ").arg(QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss.z"));

    #ifdef WIN32
        // Enable colored output for
        // Set output mode to handle virtual terminal sequences
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    #endif

    m_mutex.lock();
    switch (type) {
        case QtWarningMsg:
            LOG_WRITE(std::cout, COLOR_WARN, QString("[WARN%1").arg(sDate).toStdString(), msg.toStdString());
        break;
        case QtCriticalMsg:
            LOG_WRITE(std::cout, COLOR_CRITICAL, QString("[CRIT%1").arg(sDate).toStdString(), msg.toStdString());
            break;
        case QtFatalMsg:
            LOG_WRITE(std::cout, COLOR_FATAL, QString("[FATAL%1").arg(sDate).toStdString(), msg.toStdString());
            break;
        case QtDebugMsg:
            LOG_WRITE(std::cout, COLOR_DEBUG, QString("[DEBUG%1").arg(sDate).toStdString(), msg.toStdString());
            break;
        case QtInfoMsg:
            LOG_WRITE(std::cout, COLOR_INFO, QString("[INFO%1").arg(sDate).toStdString(), msg.toStdString());
            break;
        default:
            LOG_WRITE(std::cout, COLOR_RESET, "", msg.toStdString());
            break;
    }
    m_mutex.unlock();
}
