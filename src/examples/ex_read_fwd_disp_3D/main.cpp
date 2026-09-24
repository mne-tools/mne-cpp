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
 * @brief    Example of reading ForwardSolution data from a fiff file and display it in 3D
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp3D/view/brainview.h>
#include <disp3D/model/braintreemodel.h>

#include <fs/fs_surfaceset.h>
#include <fs/fs_annotationset.h>
#include <mne/mne_forward_solution.h>
#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QVector3D>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace MNELIB;
using namespace UTILSLIB;
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

    qInstallMessageHandler(MNELogger::customLogWriter);
    QApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Read Forward and Visualize in 3D Example");
    parser.addHelpOption();

    QCommandLineOption fwdFileOption("fwd", "Path to the forward solution <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif");
    QCommandLineOption surfOption("surfType", "FsSurface type <type>.", "type", "orig");
    QCommandLineOption annotOption("annotType", "FsAnnotation type <type>.", "type", "aparc.a2009s");
    QCommandLineOption subjectOption("subject", "Selected subject <subject>.", "subject", "sample");
    QCommandLineOption subjectPathOption("subjectPath", "Selected subject path <subjectPath>.", "subjectPath", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects");
    QCommandLineOption hemiOption("hemi", "Selected hemisphere <hemi>.", "hemi", "2");

    parser.addOption(fwdFileOption);
    parser.addOption(surfOption);
    parser.addOption(annotOption);
    parser.addOption(subjectOption);
    parser.addOption(subjectPathOption);
    parser.addOption(hemiOption);

    parser.process(a);

    //Load data
    QFile t_File(parser.value(fwdFileOption));
    MNEForwardSolution t_forwardSolution(t_File);

    BrainView *pBrainView = new BrainView();
    BrainTreeModel *pModel = new BrainTreeModel();
    pBrainView->setModel(pModel);

    // Load surfaces from subject directory
    FsSurfaceSet t_surfSet(parser.value(subjectOption), parser.value(hemiOption).toInt(), parser.value(surfOption), parser.value(subjectPathOption));
    FsAnnotationSet t_annotationSet(parser.value(subjectOption), parser.value(hemiOption).toInt(), parser.value(annotOption), parser.value(subjectPathOption));

    for (auto it = t_surfSet.data().constBegin(); it != t_surfSet.data().constEnd(); ++it) {
        int hIdx = it.key();
        QString hemi = (it.value().hemi() == 0) ? "lh" : "rh";
        QString surfType = it.value().surf().isEmpty() ? "inflated" : it.value().surf();
        pModel->addSurface(parser.value(subjectOption), hemi, surfType, it.value());
        if (t_annotationSet.size() > hIdx)
            pModel->addAnnotation(parser.value(subjectOption), hemi, t_annotationSet[hIdx]);
    }

    // Load source space from forward solution file for visualization
    pBrainView->loadSourceSpace(parser.value(fwdFileOption));

    //Visualize result in 3D
    pBrainView->show();

    return a.exec();
}
