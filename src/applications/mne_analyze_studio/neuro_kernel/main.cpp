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

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);

    MNEANALYZESTUDIO::NeuroKernelService service;
    if(!service.start("mne_analyze_studio.neuro_kernel")) {
        return 1;
    }

    return application.exec();
}
