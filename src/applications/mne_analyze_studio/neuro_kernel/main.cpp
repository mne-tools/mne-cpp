//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Entry point for the MNE Analyze Studio Neuro-Kernel process.
 */

#include "neurokernelservice.h"

#include <QCoreApplication>
#include <QSocketNotifier>

#include <csignal>

#include <unistd.h>

namespace
{

int g_signalPipe[2] = {-1, -1};

void handleUnixSignal(int signalValue)
{
    const char signalByte = static_cast<char>(signalValue);
    if(g_signalPipe[1] >= 0) {
        ::write(g_signalPipe[1], &signalByte, sizeof(signalByte));
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
        ::read(g_signalPipe[0], &signalByte, sizeof(signalByte));
        application.quit();
        notifier->setEnabled(true);
    });

    std::signal(SIGINT, handleUnixSignal);
    std::signal(SIGTERM, handleUnixSignal);
}

}

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    installUnixSignalHandlers(application);

    MNEANALYZESTUDIO::NeuroKernelService service;
    if(!service.start("mne_analyze_studio.neuro_kernel")) {
        return 1;
    }

    return application.exec();
}
