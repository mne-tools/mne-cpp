//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 * @brief    MNE Inspect application entry point.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QCommandLineParser>

#include <utils/generics/mne_logger.h>

#include "app/mainwindow.h"

using namespace UTILSLIB;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("MNE Inspect");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);
    QCoreApplication::setOrganizationName("MNE-CPP");
    QCoreApplication::setOrganizationDomain("mne-cpp.org");

    QCommandLineParser parser;
    parser.setApplicationDescription("MNE Inspect - Brain Visualization & Source Analysis");
    parser.addHelpOption();

    QCommandLineOption subjectPathOption("subjectPath", "Path to subjects directory", "path",
        QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects");
    QCommandLineOption subjectOption("subject", "Subject name", "name", "sample");
    QCommandLineOption hemiOption("hemi", "Hemisphere (unused)", "hemi", "0");
    QCommandLineOption bemOption("bem", "BEM file path", "path", "");
    QCommandLineOption transOption("trans", "Transformation file path", "path", "");
    QCommandLineOption stcOption("stc", "Source estimate file path (may be repeated)", "path");
    QCommandLineOption digitizerOption("digitizer", "Digitizer/sensor file path", "path", "");
    QCommandLineOption srcSpaceOption("srcSpace", "Source space / forward solution file path", "path", "");
    QCommandLineOption atlasOption("atlas", "Atlas annotation file path (lh or rh, sibling auto-detected)", "path", "");
    QCommandLineOption evokedOption("evoked", "Evoked/average file path", "path", "");
    QCommandLineOption mriOption("mri", "MRI volume file path (MGH/MGZ/NIfTI)", "path", "");

    parser.addOptions({subjectPathOption, subjectOption, hemiOption, bemOption, transOption, stcOption, digitizerOption, srcSpaceOption, atlasOption, evokedOption, mriOption});
    parser.process(app);

    MainWindow mainWindow;
    mainWindow.loadInitialData(
        parser.value(subjectPathOption),
        parser.value(subjectOption),
        parser.value(bemOption),
        parser.value(transOption),
        parser.values(stcOption),
        parser.value(digitizerOption),
        parser.value(srcSpaceOption),
        parser.value(atlasOption),
        parser.value(evokedOption),
        parser.value(mriOption)
    );
    mainWindow.show();

    return app.exec();
}
