//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) %{Year} MNE-CPP Authors
 *
 * @file     %{CppFileName}
 * @author   %{author} <%{eMail}>
 * @since    0.1.0
 * @date     %{Month} %{Year}
 * @brief    %{exDescription}.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/applicationlogger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QCommandLineParser>

//=============================================================================================================
// Eigen
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

//=============================================================================================================
// MAIN
//=============================================================================================================

//=============================================================================================================
/**
 * The function main marks the entry point of the program.
 * By default, main has the storage class extern.
 *
 * @param [in] argc (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
 * @param [in] argv (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
 * @return the value that was set to exit() (which is 0 if exit() is called via quit()).
 */
int main(int argc, char *argv[])
{
    qInstallMessageHandler(UTILSLIB::ApplicationLogger::customLogWriter);
    QCoreApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Example name");
    parser.addHelpOption();

    QCommandLineOption parameterOption("parameter", "The first parameter description.");

    parser.addOption(parameterOption);

    parser.process(a);

    // Add exampel code here

    return a.exec();
}
