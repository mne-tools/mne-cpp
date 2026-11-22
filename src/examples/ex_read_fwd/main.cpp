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
 * @brief     Read a forward solution, print its gain matrix and source geometry, and cluster it by cortical parcellation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_forward_solution.h>
#include <fs/fs_annotationset.h>
#include <math/matrix_io.h>
#include <base/log.h>
#include <base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QCommandLineParser>
#include <QDir>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

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
    QCoreApplication app(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Read Forward Example");
    parser.addHelpOption();

    QCommandLineOption fwdFileOption("fwd", "Path to the forward solution <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif");
    QCommandLineOption surfOption("surfType", "FsSurface type <type>.", "type", "orig");
    QCommandLineOption annotOption("annotType", "FsAnnotation type <type>.", "type", "aparc");
    QCommandLineOption subjectOption("subject", "Selected subject <subject>.", "subject", "sample");
    QCommandLineOption subjectPathOption("subjectPath", "Selected subject path <subjectPath>.", "subjectPath", QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/subjects");
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
    MNEForwardSolution t_Fwd(parser.value(fwdFileOption).toStdU16String());
    if (t_Fwd.empty()) {
        Log::warning("Could not read the forward solution {}", t_fileForwardSolution.fileName().toStdString());
        return 1;
    }

    if (t_Fwd.source_ori != -1) {
        std::cout << "\nfirst 10 rows and columns of the Gain Matrix:\n"
                  << t_Fwd.sol.data.block(0, 0, 10, 10) << std::endl;
        std::cout << "\nfirst 10 dipole coordinates:\n"
                  << t_Fwd.source_rr.block(0, 0, 10, 3) << std::endl;
        std::cout << "\nfirst 10 dipole normales:\n"
                  << t_Fwd.source_nn.block(0, 0, 10, 3) << std::endl;
    }

    // === Option to cluster forward solution ===
    FsAnnotationSet t_annotationSet(parser.value(subjectOption).toStdString(), parser.value(hemiOption).toInt(), parser.value(annotOption).toStdString(), parser.value(subjectPathOption).toStdU16String());

    //
    // Cluster forward solution;
    //
    MNEForwardSolution t_clusteredFwd = t_Fwd.cluster_forward_solution(t_annotationSet, 20); //40);

    Log::debug("==== Results ====");
    Log::debug("nrow: {}", t_clusteredFwd.sol.nrow);
    Log::debug("ncol: {}", t_clusteredFwd.sol.ncol);
    Log::debug("row_names:");
    Log::debug("{}", join(t_clusteredFwd.sol.row_names, " "));
    Log::debug("col_names:");
    Log::debug("{}", join(t_clusteredFwd.sol.col_names, " "));
    Log::debug("write fwd data to ./test_fwd.txt ...");
    mne::write_matrix(t_clusteredFwd.sol.data, QDir::temp().filePath("ex_read_fwd-clustered.txt").toStdString());
    Log::debug("[done]");

    return t_clusteredFwd.sol.ncol > 0 && t_clusteredFwd.sol.ncol < t_Fwd.sol.ncol ? 0 : 1;
}
