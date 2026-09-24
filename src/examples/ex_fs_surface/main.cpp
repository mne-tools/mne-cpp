//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July, 2012
 * @brief    Example of an FreeSurfer FsSurface application
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp3D/view/brainview.h>
#include <disp3D/model/braintreemodel.h>

#include <fs/fs_surfaceset.h>

#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QCommandLineParser>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;

//=============================================================================================================
// MAIN
//=============================================================================================================

//=============================================================================================================
/**
 * The function main marks the entry point of the program.
 * By default, main has the storage class extern.
 *
 * @param[in] argc (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
 * @param[in] argv (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
 * @return the value that was set to exit() (which is 0 if exit() is called via quit()).
 */
int main(int argc, char *argv[])
{
    #ifdef STATICBUILD
    // Q_INIT_RESOURCE(mne_disp3d);
    #endif
    
    qInstallMessageHandler(UTILSLIB::MNELogger::customLogWriter);
    QApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Fs FsSurface Example");
    parser.addHelpOption();

    QCommandLineOption hemiOption("hemi", "Selected hemisphere <hemi>.", "hemi", "2");
    QCommandLineOption subjectOption("subject", "Selected subject <subject>.", "subject", "sample");
    QCommandLineOption subjectPathOption("subjectPath", "Selected subject path <subjectPath>.", "subjectPath", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects");

    parser.addOption(hemiOption);
    parser.addOption(subjectOption);
    parser.addOption(subjectPathOption);

    parser.process(a);

    int hemi = parser.value(hemiOption).toInt();
    QString subject = parser.value(subjectOption);
    QString subjectPath = parser.value(subjectPathOption);

    //
    // pial
    //
    FsSurfaceSet tSurfSetPial (subject, hemi, "pial", subjectPath);

    BrainView *pBrainView = new BrainView();
    BrainTreeModel *pModel = new BrainTreeModel();
    pBrainView->setModel(pModel);

    for (auto it = tSurfSetPial.data().constBegin(); it != tSurfSetPial.data().constEnd(); ++it) {
        QString sHemi = (it.value().hemi() == 0) ? "lh" : "rh";
        pModel->addSurface(subject, sHemi, "pial", it.value());
    }

    //
    // inflated
    //
    FsSurfaceSet tSurfSetInflated (subject, hemi, "inflated", subjectPath);
    for (auto it = tSurfSetInflated.data().constBegin(); it != tSurfSetInflated.data().constEnd(); ++it) {
        QString sHemi = (it.value().hemi() == 0) ? "lh" : "rh";
        pModel->addSurface(subject, sHemi, "inflated", it.value());
    }

    //
    // orig
    //
    FsSurfaceSet tSurfSetOrig (subject, hemi, "orig", subjectPath);
    for (auto it = tSurfSetOrig.data().constBegin(); it != tSurfSetOrig.data().constEnd(); ++it) {
        QString sHemi = (it.value().hemi() == 0) ? "lh" : "rh";
        pModel->addSurface(subject, sHemi, "orig", it.value());
    }

    //
    // white
    //
    FsSurfaceSet tSurfSetWhite (subject, hemi, "white", subjectPath);
    for (auto it = tSurfSetWhite.data().constBegin(); it != tSurfSetWhite.data().constEnd(); ++it) {
        QString sHemi = (it.value().hemi() == 0) ? "lh" : "rh";
        pModel->addSurface(subject, sHemi, "white", it.value());
    }

    pBrainView->show();

    return a.exec();
}
