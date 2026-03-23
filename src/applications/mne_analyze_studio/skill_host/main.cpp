//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Entry point for the MNE Analyze Studio skill-host process.
 */

#include "skillhostservice.h"

#include <QCoreApplication>

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    MNEANALYZESTUDIO::SkillHostService service;
    Q_UNUSED(service)
    return application.exec();
}
