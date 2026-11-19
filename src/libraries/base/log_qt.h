//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     log_qt.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Connects @ref mne::Log and Qt's message handler, for Qt programs only.
 *
 * Header-only, so mne_base itself does not link Qt; only a program that includes this header and links QtCore
 * uses it. Use one direction per program:
 *  - install_qt_message_handler(): Qt's own messages (qDebug(), qWarning(), ...) go through @ref mne::Log, so a
 *    Qt application prints everything one way.
 *  - forward_log_to_qt(): Log messages go to Qt's message handler, e.g. so QTest::ignoreMessage() sees them.
 */

#ifndef BASE_LOG_QT_H
#define BASE_LOG_QT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "log.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QString>
#include <QtGlobal>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string_view>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Installs a Qt message handler that writes every Qt message through @ref mne::Log::write, so it is printed (or
 * passed to the sink) like a library message, at or above Log::level(). Do not combine with forward_log_to_qt().
 */
inline void install_qt_message_handler()
{
    qInstallMessageHandler([](QtMsgType type, const QMessageLogContext& /*context*/, const QString& message) {
        Log::Level level = Log::Level::Debug;
        switch (type) {
            case QtDebugMsg:
                level = Log::Level::Debug;
                break;
            case QtInfoMsg:
                level = Log::Level::Info;
                break;
            case QtWarningMsg:
                level = Log::Level::Warning;
                break;
            case QtCriticalMsg:
            case QtFatalMsg:
                level = Log::Level::Critical;
                break;
        }
        Log::write(level, message.toStdString());
    });
}

//=============================================================================================================
/**
 * Makes Qt's message handler the sink of @ref mne::Log, so debug, info, warning and critical messages become qDebug(),
 * qInfo(), qWarning() and qCritical(). Do not combine with install_qt_message_handler().
 *
 * @return The sink that was installed before.
 */
inline Log::Sink forward_log_to_qt()
{
    return Log::set_sink([](Log::Level level, std::string_view message) {
        const QString text = QString::fromUtf8(message.data(), static_cast<qsizetype>(message.size()));
        switch (level) {
            case Log::Level::Debug:
                qDebug().noquote() << text;
                break;
            case Log::Level::Info:
                qInfo().noquote() << text;
                break;
            case Log::Level::Warning:
                qWarning().noquote() << text;
                break;
            case Log::Level::Critical:
                qCritical().noquote() << text;
                break;
        }
    });
}

} // namespace mne

#endif // BASE_LOG_QT_H
