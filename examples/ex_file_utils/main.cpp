//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.9
 * @date     September, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Gabriel Motta. All rights reserved.
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
 * @brief    Average data from a raw data file
 *
 */
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/files.h>

#include <chrono>
#include <string>
#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDir>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

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
    QCoreApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("File Utils Example");
    parser.addHelpOption();

    QCommandLineOption stepOption("step", "Steps through each command waiting for input before proceeding.", "pickAll", "true");
    parser.addOption(stepOption);

    parser.process(a);

    bool bStep = (parser.value("step") == "true");

    std::string sDirPath = QCoreApplication::applicationDirPath().toStdString();
    std::string sFilePath = sDirPath + "/test.txt";

    std::cout << "=====  Creating File  =====\n";
    if(bStep){
        std::cout << "Press RETURN to execute.\n";
        std::cin.get();
    }
    Files::create(sFilePath);

    std::cout << "=====  Checking File  =====\n";
    if(bStep){
        std::cout << "Press RETURN to execute.\n";
        std::cin.get();
    }
    std::string answer;
    if (Files::exists(sFilePath)){
        answer = "Yes.";
    } else {
        answer = "No.";
    }
    std::cout << "Does file exist? " << answer << "\n";

    std::cout << "=====  Copying File   =====\n";
    if(bStep){
        std::cout << "Press RETURN to execute.\n";
        std::cin.get();
    }

    std::string sFilePath2 = sDirPath + "/test_copy.txt";
    std::string sFilePath3 = sDirPath + "/another_test_copy.txt";

    Files::copy(sFilePath, sFilePath2);
    Files::copy(sFilePath2, sFilePath);

    std::cout << "=====  Renaming File  =====\n";
    if(bStep){
        std::cout << "Press RETURN to execute.\n";
        std::cin.get();
    }
    std::string sFilePath4 = sDirPath + "/another_test_copy.txt";

    Files::rename(sFilePath3, sFilePath4);

    return a.exec();
}
