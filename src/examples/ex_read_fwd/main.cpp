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
 * @brief     Example of reading ForwardSolution data from a fiff file
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <mne/mne_forward_solution.h>
#include <fs/fs_annotationset.h>
#include <utils/ioutils.h>
#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QDebug>
#include <QCommandLineParser>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

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
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Read Forward Example");
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

    parser.process(app);

    //Load data
    QFile t_fileForwardSolution(parser.value(fwdFileOption));
    MNEForwardSolution t_Fwd(t_fileForwardSolution);

    if(t_Fwd.source_ori != -1)
    {
        std::cout << "\nfirst 10 rows and columns of the Gain Matrix:\n" << t_Fwd.sol->data.block(0,0,10,10) << std::endl;
        std::cout << "\nfirst 10 dipole coordinates:\n" << t_Fwd.source_rr.block(0,0,10,3) << std::endl ;
        std::cout << "\nfirst 10 dipole normales:\n" << t_Fwd.source_nn.block(0,0,10,3) << std::endl ;
    }

    // === Option to cluster forward solution ===
    FsAnnotationSet t_annotationSet (parser.value(subjectOption), parser.value(hemiOption).toInt(), parser.value(annotOption), parser.value(subjectPathOption));

    //
    // Cluster forward solution;
    //
    MNEForwardSolution t_clusteredFwd = t_Fwd.cluster_forward_solution(t_annotationSet, 20);//40);

    qDebug() << "==== Results ====";
    qDebug() << "nrow: " << t_clusteredFwd.sol->nrow;
    qDebug() << "ncol: " << t_clusteredFwd.sol->ncol;
    qDebug() << "row_names:";
    qDebug() << t_clusteredFwd.sol->row_names;
    qDebug() << "col_names:";
    qDebug() << t_clusteredFwd.sol->col_names;
    qDebug() << "write fwd data to ./test_fwd.txt ...";
    IOUtils::write_eigen_matrix(t_clusteredFwd.sol->data, QString("./test_fwd.txt"));
    qDebug() << "[done]";

    return app.exec();
}
