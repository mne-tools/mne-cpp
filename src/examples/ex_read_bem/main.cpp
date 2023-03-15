//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     Mai, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Example of reading BEM data
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>

#include <mne/mne.h>

#include <utils/ioutils.h>
#include <utils/generics/applicationlogger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QCommandLineParser>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

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
    parser.setApplicationDescription("Read BEM Example");
    parser.addHelpOption();
    QCommandLineOption bemFileInOption("bem", "Path to BEM <file>.", "file", QCoreApplication::applicationDirPath() + "/../data/MNE-sample-data/subjects/sample/bem/sample-head.fif");
    QCommandLineOption bemFileOutOption("bemOut", "Path to BEM <file>, which is to be written.", "file", "./sample-head-test.fif");

//    QCoreApplication::applicationDirPath() + "/../data/MNE-sample-data/subjects/sample/bem/sample-5120-5120-5120-bem.fif"
//    QCoreApplication::applicationDirPath() + "/../data/MNE-sample-data/subjects/sample/bem/sample-all-src.fif"
//    QCoreApplication::applicationDirPath() + "/../data/MNE-sample-data/subjects/sample/bem/sample-5120-bem-sol.fif"
//    QCoreApplication::applicationDirPath() + "/../data/MNE-sample-data/subjects/sample/bem/sample-5120-bem.fif"

    parser.addOption(bemFileInOption);
    parser.addOption(bemFileOutOption);
    parser.process(app);

    // Read the BEM
    QFile t_fileBem(parser.value(bemFileInOption));
    MNEBem t_Bem(t_fileBem);

    if( t_Bem.size() > 0 )
    {
        qDebug() << "Loaded BEM";
        qDebug() << "t_Bem[0].tri_nn:" << t_Bem[0].tri_nn(0,0) << t_Bem[0].tri_nn(0,1) << t_Bem[0].tri_nn(0,2);
        qDebug() << "t_Bem[0].tri_nn:" << t_Bem[0].tri_nn(2,0) << t_Bem[0].tri_nn(2,1) << t_Bem[0].tri_nn(2,2);
        qDebug() << "t_Bem[0].rr:" << t_Bem[0].rr(2,0) << t_Bem[0].rr(2,1) << t_Bem[0].rr(2,2);
    }

    // Write the BEM
    QFile t_fileBemTest(parser.value(bemFileOutOption));
    t_Bem.write(t_fileBemTest);
    t_fileBemTest.close();

    MNEBem t_BemTest (t_fileBemTest) ;

    if( t_BemTest.size() > 0 )
    {
        qDebug() << "Loaded written BEM";
        qDebug() << "t_BemTest[0].tri_nn:" << t_BemTest[0].tri_nn(0,0) << t_BemTest[0].tri_nn(0,1) << t_BemTest[0].tri_nn(0,2);
        qDebug() << "t_BemTest[0].tri_nn:" << t_BemTest[0].tri_nn(2,0) << t_BemTest[0].tri_nn(2,1) << t_BemTest[0].tri_nn(2,2);
        qDebug() << "t_BemTest[0].rr:" << t_BemTest[0].rr(2,0) << t_BemTest[0].rr(2,1) << t_BemTest[0].rr(2,2);
    }

    return app.exec();
}
