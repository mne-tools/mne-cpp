//=============================================================================================================
/**
 * @file     mne_logger.cpp
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
 * @brief    Contains the definition of the MNELogger class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <string>
#include "mne_logger.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtGlobal>
#include <QDateTime>
#include <QMutexLocker>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;

//=============================================================================================================
// Definitions
//=============================================================================================================

#ifdef WIN32
    #include <wchar.h>
    #include <windows.h>
    #define COLOR_INFO          "\033[32;1m"
    #define COLOR_DEBUG         "\033[32;1m"
    #define COLOR_WARN          "\033[33;1m"
    #define COLOR_CRITICAL      "\033[31;1m"
    #define COLOR_FATAL         "\033[33;1m"
    #define COLOR_RESET         "\033[0m"
#else
    #define COLOR_INFO          "\033[32;1m"        //green
    #define COLOR_DEBUG         "\033[32;1m"        //green
    #define COLOR_WARN          "\033[33;1m"        //yellow
    #define COLOR_CRITICAL      "\033[31;1m"        //red
    #define COLOR_FATAL         "\033[33;1m"        //yellow
    #define COLOR_RESET         "\033[0m"           //reset
#endif

std::mutex MNELogger::m_mutex;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void MNELogger::customLogWriter(QtMsgType type,
                                        const QMessageLogContext &context,
                                        const QString &msg)
{
    Q_UNUSED(context)

    // Map Qt message type to numeric level (must match CMake MNE_LOG_LEVEL values)
    // 0=DEBUG, 1=INFO, 2=WARN, 3=CRIT, 4=FATAL
    int msgLevel = 0;
    switch(type) {
        case QtDebugMsg:    msgLevel = 0; break;
        case QtInfoMsg:     msgLevel = 1; break;
        case QtWarningMsg:  msgLevel = 2; break;
        case QtCriticalMsg: msgLevel = 3; break;
        case QtFatalMsg:    msgLevel = 4; break;
        default:            msgLevel = 0; break;
    }

#ifdef MNE_LOG_LEVEL
    if (msgLevel < MNE_LOG_LEVEL)
        return;
#endif

    std::string text = msg.toStdString();

    std::lock_guard<std::mutex> lock(m_mutex);

#ifdef MNE_LOG_MODE_FORMATTED
    static bool s_startOfLine = true;

    // Determine color and level tag
    const char* color = COLOR_RESET;
    const char* level = "";
    switch(type) {
        case QtDebugMsg:    color = COLOR_DEBUG;    level = "[DEBUG] "; break;
        case QtInfoMsg:     color = COLOR_INFO;     level = "[INFO] ";  break;
        case QtWarningMsg:  color = COLOR_WARN;     level = "[WARN] ";  break;
        case QtCriticalMsg: color = COLOR_CRITICAL; level = "[CRIT] ";  break;
        case QtFatalMsg:    color = COLOR_FATAL;    level = "[FATAL] "; break;
        default:                                    level = "";         break;
    }

    // Uncomment the following line to include date/time in log prefix:
    // std::string sDateTime = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss.z ").toStdString();
    std::string sDateTime;

    std::string prefix = std::string(color) + sDateTime + level + COLOR_RESET;

    // Walk through the message character by character.
    // Print the prefix only at the start of each new line.
    for (size_t i = 0; i < text.size(); ++i) {
        if (s_startOfLine && text[i] != '\n') {
            std::cout << prefix;
            s_startOfLine = false;
        }
        std::cout << text[i];
        if (text[i] == '\n') {
            s_startOfLine = true;
        }
    }

    if (text.empty() || text.back() != '\n') {
        std::cout << '\n';
        s_startOfLine = true;
    }
#else
    // PLAIN mode: raw text, like printf
    std::cout << text;
    if (text.empty() || text.back() != '\n') {
        std::cout << '\n';
    }
#endif

    std::cout.flush();
}
