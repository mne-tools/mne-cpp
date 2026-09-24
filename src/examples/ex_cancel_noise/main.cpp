//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July, 2016
 * @brief    Example of noise cancellation procedures
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <math.h>

#include <utils/generics/mne_logger.h>

#include <fiff/fiff.h>
#include <mne/mne.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QCommandLineParser>
#include <QFile>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace MNELIB;
using namespace UTILSLIB;

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
    parser.setApplicationDescription("Cancel Noise Example");
    parser.addHelpOption();

    QCommandLineOption evokedFileOption("ave", "Path to the evoked/average <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    QCommandLineOption destCompsOption("destComps", "<Destination> of the compensator which is to be calculated.", "destination", "2");

    parser.addOption(evokedFileOption);
    parser.addOption(destCompsOption);

    parser.process(app);

    //generate FiffEvokedSet
    QFile t_sampleFile(parser.value(evokedFileOption));
    FiffEvokedSet p_FiffEvokedSet(t_sampleFile);

    //cancelNoise example
    qint32 comp_now = p_FiffEvokedSet.info.get_current_comp();
    qint32 dest_comp = parser.value(destCompsOption).toInt();

    if(comp_now != dest_comp)
        p_FiffEvokedSet.compensate_to(p_FiffEvokedSet,dest_comp);

    //example for compensator generation
    FiffCtfComp comp;
    if(dest_comp > 0 )
    {
        qDebug() << "This part needs to be debugged";
        p_FiffEvokedSet.info.make_compensator(comp_now,dest_comp,comp); //ToDo: make_compensator needs to be debugged
        printf("Appropriate forward operator compensator created.\n");
    }
    else
        printf("No forward operator compensator needed.\n");

    //Do the projection
    //this was already performed with the FiffEvoked instantiation

    return app.exec();
}

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================
