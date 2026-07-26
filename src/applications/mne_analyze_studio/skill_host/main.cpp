//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *

 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Entry point for the MNE Analyze Studio skill-host process.
 */

#include "skillhostservice.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSocketNotifier>
#include <QtGlobal>

#include <csignal>

#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

namespace
{

#ifdef Q_OS_UNIX
int g_signalPipe[2] = {-1, -1};

void handleUnixSignal(int signalValue)
{
    const char signalByte = static_cast<char>(signalValue);
    if(g_signalPipe[1] >= 0) {
        //
        // Only async-signal-safe calls are allowed here, so there is nothing
        // useful to do if the pipe is full or the write is interrupted: the
        // notifier will still see whichever byte arrived. Consume the result
        // explicitly rather than discarding it.
        //
        const ssize_t written = ::write(g_signalPipe[1], &signalByte, sizeof(signalByte));
        static_cast<void>(written);
    }
}

void installUnixSignalHandlers(QCoreApplication& application)
{
    if(::pipe(g_signalPipe) != 0) {
        return;
    }

    auto* notifier = new QSocketNotifier(g_signalPipe[0], QSocketNotifier::Read, &application);
    QObject::connect(notifier, &QSocketNotifier::activated, &application, [&application, notifier](int) {
        notifier->setEnabled(false);
        char signalByte = 0;
        //
        // Drain the byte the handler wrote. If the read fails there is no
        // signal to act on, so leave the application running rather than
        // quitting on a spurious wakeup.
        //
        const ssize_t bytesRead = ::read(g_signalPipe[0], &signalByte, sizeof(signalByte));
        if(bytesRead > 0) {
            application.quit();
        }
        notifier->setEnabled(true);
    });

    std::signal(SIGINT, handleUnixSignal);
    std::signal(SIGTERM, handleUnixSignal);
}
#else
void installUnixSignalHandlers(QCoreApplication&)
{
}
#endif

QString resolveStudioExtensionsDirectory()
{
    const QString relativePath = "src/applications/mne_analyze_studio/extensions";
    const QStringList seedDirectories{
        QDir::currentPath(),
        QCoreApplication::applicationDirPath()
    };

    for(const QString& seedDirectory : seedDirectories) {
        QDir searchDir(seedDirectory);
        for(int depth = 0; depth < 8; ++depth) {
            const QString candidate = searchDir.filePath(relativePath);
            QFileInfo candidateInfo(candidate);
            if(candidateInfo.exists() && candidateInfo.isDir()) {
                return candidateInfo.absoluteFilePath();
            }

            if(!searchDir.cdUp()) {
                break;
            }
        }
    }

    return QDir::current().filePath(relativePath);
}

}

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    installUnixSignalHandlers(application);
    MNEANALYZESTUDIO::SkillHostService service;

    const QString extensionsDirectory = resolveStudioExtensionsDirectory();
    if(!service.start("mne_analyze_studio.extension_host", extensionsDirectory)) {
        return 1;
    }

    return application.exec();
}
