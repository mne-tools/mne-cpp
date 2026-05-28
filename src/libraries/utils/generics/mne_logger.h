//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_logger.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Custom Qt message handler that adds ANSI colour, timestamps and source-location tags to qDebug/qWarning output.
 *
 * @ref UTILSLIB::MNELogger installs a single @c QtMessageHandler
 * via @c qInstallMessageHandler so every @c qDebug, @c qInfo,
 * @c qWarning, @c qCritical and @c qFatal call across mne-cpp
 * is formatted consistently and serialised through one
 * @c std::mutex — necessary because Qt's default handler is not
 * thread-safe and acquisition plugins emit log lines from many
 * worker threads simultaneously.
 *
 * Each line is prefixed with the message type (colourised on
 * TTYs that report @c xterm-256color) and the originating
 * file/line/function so log triage in user-submitted bug
 * reports does not require attaching a debugger.
 */

#ifndef MNE_LOGGER_H
#define MNE_LOGGER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../utils_global.h"
#include <mutex>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QMutex>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * The Apllicationlogger provides colorized keywords in terminal outputs for better overview
 *
 * @brief Custom Qt message handler that formats and routes qDebug/qWarning/qCritical output to file or console.
 */
class UTILSSHARED_EXPORT MNELogger
{
public:
    //=========================================================================================================
    /**
     * Customized logWriter to colorize type of the message in the terminal
     *
     * @param[in] type      Enum to identify the various message types (qDebug, qInfo, qCritical, qWarning and qFatal).
     * @param[in] context   Context provides information about the source code location.
     * @param[in] msg       The message to print in the terminal.
     */
    static void customLogWriter(QtMsgType type, const QMessageLogContext &context, const QString &msg);

private:
    static std::mutex m_mutex;
};
}
#endif // MNE_LOGGER_H
