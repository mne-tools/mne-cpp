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
 * @brief    Read a forward solution and show its source space with the FreeSurfer parcellation in 3D.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp3D/view/brainview.h>
#include <disp3D/model/braintreemodel.h>

#include <fs/fs_surfaceset.h>
#include <fs/fs_annotationset.h>
#include <mne/mne_forward_solution.h>
#include <base/log.h>
#include <base/log_qt.h>

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

using namespace mne;
using namespace mne::disp;

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
int main(int argc, char* argv[])
{
#ifdef STATICBUILD
// Q_INIT_RESOURCE(mne_disp3d);
#endif

    mne::install_qt_message_handler();
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
    MNEForwardSolution t_forwardSolution(parser.value(fwdFileOption).toStdU16String());

    BrainView* pBrainView = new BrainView();
    BrainTreeModel* pModel = new BrainTreeModel();
    pBrainView->setModel(pModel);

    // Load surfaces from subject directory
    FsSurfaceSet t_surfSet(parser.value(subjectOption).toStdString(), parser.value(hemiOption).toInt(), parser.value(surfOption).toStdString(), parser.value(subjectPathOption).toStdU16String());
    FsAnnotationSet t_annotationSet(parser.value(subjectOption).toStdString(), parser.value(hemiOption).toInt(), parser.value(annotOption).toStdString(), parser.value(subjectPathOption).toStdU16String());

    for (auto it = t_surfSet.data().cbegin(); it != t_surfSet.data().cend(); ++it) {
        int hIdx = it->first;
        QString hemi = (it->second.hemi() == 0) ? "lh" : "rh";
        QString surfType = QString::fromStdString(it->second.surf().empty() ? "inflated" : it->second.surf());
        pModel->addSurface(parser.value(subjectOption), hemi, surfType, it->second);
        if (t_annotationSet.size() > hIdx)
            pModel->addAnnotation(parser.value(subjectOption), hemi, t_annotationSet[hIdx]);
    }

    // Load source space from forward solution file for visualization
    pBrainView->loadSourceSpace(parser.value(fwdFileOption));

    //Visualize result in 3D
    pBrainView->show();

    return a.exec();
}
