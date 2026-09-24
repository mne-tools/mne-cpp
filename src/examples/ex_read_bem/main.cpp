//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     Mai, 2015
 * @brief    Example of reading BEM data
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>

#include <mne/mne.h>

#include <utils/ioutils.h>
#include <utils/generics/mne_logger.h>

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
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Read BEM Example");
    parser.addHelpOption();
    QCommandLineOption bemFileInOption("bem", "Path to BEM <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects/sample/bem/sample-head.fif");
    QCommandLineOption bemFileOutOption("bemOut", "Path to BEM <file>, which is to be written.", "file", "./sample-head-test.fif");

//    QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects/sample/bem/sample-5120-5120-5120-bem.fif"
//    QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects/sample/bem/sample-all-src.fif"
//    QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects/sample/bem/sample-5120-bem-sol.fif"
//    QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects/sample/bem/sample-5120-bem.fif"

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
