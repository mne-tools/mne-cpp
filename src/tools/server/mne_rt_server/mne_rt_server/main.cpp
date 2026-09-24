//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 * @brief    Implements the main() application function.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>

#include <utils/generics/mne_logger.h>

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
    qInstallMessageHandler(UTILSLIB::MNELogger::customLogWriter);
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
        QFile file(QCoreApplication::applicationDirPath() + "/../resources/mne_rt_server/plugins/fiffsimulator/FiffSimulation.cfg");

        if (QFileInfo(parser.value(inFileOpt)).exists()) {
            if (file.open(QIODevice::Truncate | QIODevice::Text | QIODevice::WriteOnly)) {
                QTextStream stream(&file);
                stream << QString("simFile = %1").arg(parser.value(inFileOpt));;
                file.close();

                qInfo() << QString("[MNERtServer::main] Streaming file %1").arg(parser.value(inFileOpt));
            } else {
                qWarning() << QString("[MNERtServer::main] Could not open %1").arg(QCoreApplication::applicationDirPath() + "/../resources/mne_rt_server/plugins/fiffsimulator/FiffSimulation.cfg");
            }
        } else {
            qWarning("[MNERtServer::main] Provided file does not exist. Falling back to default one.");

            if (file.open(QIODevice::Truncate | QIODevice::Text | QIODevice::WriteOnly)) {
                QTextStream stream(&file);
                stream << QString("simFile = <pathTo>/../resources/data/MNE-sample-data/MEG/sample/sample_audvis_raw.fif");
                file.close();
            } else {
                qWarning() << QString("[MNERtServer::main] Could not open %1").arg(QCoreApplication::applicationDirPath() + "/../resources/mne_rt_server/plugins/fiffsimulator/FiffSimulation.cfg");
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

