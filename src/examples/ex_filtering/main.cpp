//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2019-2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Ruben Doerfel <doerfelruben@aol.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     11, 2019
 * @brief     Example for filtering data with a user defined FIR filter and writing the result to a file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <math.h>

#include <fiff/fiff.h>

#include <dsp/filterkernel.h>
#include <utils/generics/mne_logger.h>

#include <dsp/rt/rt_filter.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QFile>
#include <QCommandLineParser>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace RTPROCESSINGLIB;
using namespace Eigen;

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
    QCoreApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Read Write Raw Example");
    parser.addHelpOption();

    QCommandLineOption inputOption("fileIn", "The input file <in>.", "in", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis_raw.fif");
    QCommandLineOption outputOption("fileOut", "The output file <out>.", "out", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis_filt_raw.fif");

    parser.addOption(inputOption);
    parser.addOption(outputOption);

    parser.process(a);

    // Init data loading and writing
    QFile fileIn(parser.value(inputOption));
    QFile fileOut(parser.value(outputOption));

    FiffRawData::SPtr pRaw = FiffRawData::SPtr::create(fileIn);

    // Only filter MEG and EEG channels
    RowVectorXi picks = pRaw->info.pick_types(true, false, false);

    // Filtering
    printf("Filtering...");
    if(RTPROCESSINGLIB::filterFile(fileOut,
                                   pRaw,
                                   FilterKernel::m_filterTypes.indexOf(FilterParameter("BPF")),
                                   10,
                                   10,
                                   0.1,
                                   pRaw->info.sfreq,
                                   1024,
                                   UTILSLIB::FilterKernel::m_designMethods.indexOf(FilterParameter("Cosine")),
                                   picks)) {
        printf("[done]\n");
    } else {
        printf("[failed]\n");
    }

    return 0;
}
