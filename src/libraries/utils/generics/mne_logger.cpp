//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_logger.cpp
 * @since March 2026
 * @brief Out-of-line definition of the colourised, thread-safe @c qInstallMessageHandler installed by @ref UTILSLIB::MNELogger.
 *
 * Holds the @c std::mutex used to serialise the writes to
 * @c stderr so log lines from different worker threads never
 * interleave inside one record. ANSI escape sequences are
 * emitted unconditionally; consumers that pipe the output to a
 * file or to the Windows Event Log are expected to strip them.
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
