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
#include <QDir>
#include <QFileInfo>

namespace
{

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
    MNEANALYZESTUDIO::SkillHostService service;

    const QString extensionsDirectory = resolveStudioExtensionsDirectory();
    if(!service.start("mne_analyze_studio.extension_host", extensionsDirectory)) {
        return 1;
    }

    return application.exec();
}
