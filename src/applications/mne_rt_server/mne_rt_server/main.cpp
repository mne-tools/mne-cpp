//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Christoph Dinh, Lorenz Esch, Matti Hamalainen. All rights reserved.
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
 * @brief    Implements the main() application function.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>

#include <utils/generics/applicationlogger.h>

#include "mne_rt_server.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QObject>
#include <QCommandLineParser>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTSERVER;

//=============================================================================================================
// MAIN
//=============================================================================================================

#ifdef STATICBUILD
Q_IMPORT_PLUGIN(FiffSimulator)
#endif

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
    qInstallMessageHandler(UTILSLIB::ApplicationLogger::customLogWriter);
    QCoreApplication app(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("MNE Rt Server");
    parser.addHelpOption();

    QCommandLineOption inFileOpt(QStringList() << "f" << "file",
                                 QCoreApplication::translate("main","File to stream."),
                                 QCoreApplication::translate("main","filePath"));

    parser.addOption(inFileOpt);

    parser.process(app);

    // Parse input file for mne_rt_server's FiffSimulator
    if(parser.isSet("file")) {
        QFile file(QCoreApplication::applicationDirPath() + "/resources/mne_rt_server/plugins/fiffsimulator/FiffSimulation.cfg");

        if (QFileInfo(parser.value(inFileOpt)).exists()) {
            if (file.open(QIODevice::Truncate | QIODevice::Text | QIODevice::WriteOnly)) {
                QTextStream stream(&file);
                stream << QString("simFile = %1").arg(parser.value(inFileOpt));;
                file.close();

                qInfo() << QString("[MneRtServer::main] Streaming file %1").arg(parser.value(inFileOpt));
            } else {
                qWarning() << QString("[MneRtServer::main] Could not open %1").arg(QCoreApplication::applicationDirPath() + "/resources/mne_rt_server/plugins/fiffsimulator/FiffSimulation.cfg");
            }
        } else {
            qWarning("[MneRtServer::main] Provided file does not exist. Falling back to default one.");

            if (file.open(QIODevice::Truncate | QIODevice::Text | QIODevice::WriteOnly)) {
                QTextStream stream(&file);
                stream << QString("simFile = <pathTo>/../data/MNE-sample-data/MEG/sample/sample_audvis_raw.fif");
                file.close();
            } else {
                qWarning() << QString("[MneRtServer::main] Could not open %1").arg(QCoreApplication::applicationDirPath() + "/resources/mne_rt_server/plugins/fiffsimulator/FiffSimulation.cfg");
            }
        }
    }

    MNERTServer t_MneRtServer;
    QObject::connect(&t_MneRtServer, SIGNAL(closeServer()), &app, SLOT(quit()));

    return app.exec();
}

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

