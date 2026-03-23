//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Entry point for the MNE Analyze Studio workbench process.
 */

#include "mainwindow.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);

    MNEANALYZESTUDIO::MainWindow window;
    QStringList initialFiles = application.arguments();
    if(!initialFiles.isEmpty()) {
        initialFiles.removeFirst();
    }
    window.openInitialFiles(initialFiles);
    window.show();

    return application.exec();
}
