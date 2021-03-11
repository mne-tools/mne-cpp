//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Christoph Dinh, Lorenz Esch. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Example of noise cancellation procedures
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <math.h>

#include <utils/generics/applicationlogger.h>

#include <fiff/fiff.h>
#include <mne/mne.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QCommandLineParser>

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
    qInstallMessageHandler(ApplicationLogger::customLogWriter);
    QCoreApplication app(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Cancel Noise Example");
    parser.addHelpOption();

    QCommandLineOption evokedFileOption("ave", "Path to the evoked/average <file>.", "file", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
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
