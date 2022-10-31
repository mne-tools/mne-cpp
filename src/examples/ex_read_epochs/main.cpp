//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Read epoch data from a raw data file
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <math.h>

//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <mne/mne.h>

#include <mne/mne_epoch_data_list.h>

#include <utils/generics/applicationlogger.h>

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
    qInstallMessageHandler(ApplicationLogger::customLogWriter);
    QCoreApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Read Epochs Example");
    parser.addHelpOption();

    QCommandLineOption inputOption("fileIn", "The input file <in>.", "in", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis_raw.fif");
    QCommandLineOption eventsFileOption("eve", "Path to the event <file>.", "file", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis_raw-eve.fif");
    QCommandLineOption evokedIdxOption("aveIdx", "The average <index> to choose from the average file.", "index", "3");
    QCommandLineOption pickAllOption("pickAll", "Pick all channels.", "pickAll", "true");
    QCommandLineOption keepCompOption("keepComp", "Keep compensators.", "keepComp", "false");
    QCommandLineOption destCompsOption("destComps", "<Destination> of the compensator which is to be calculated.", "destination", "0");

    parser.addOption(inputOption);
    parser.addOption(eventsFileOption);
    parser.addOption(evokedIdxOption);
    parser.addOption(pickAllOption);
    parser.addOption(keepCompOption);
    parser.addOption(destCompsOption);

    parser.process(a);

    //Load data
    QString t_fileRawName = parser.value(inputOption);
    QFile t_fileRaw(t_fileRawName);

    qint32 event = parser.value(evokedIdxOption).toInt();
    QString t_sEventName = parser.value(eventsFileOption);
    float fTMin = -1.5f;
    float fTMax = 1.5f;

    bool keep_comp = false;
    if(parser.value(keepCompOption) == "false" || parser.value(keepCompOption) == "0") {
        keep_comp = false;
    } else if(parser.value(keepCompOption) == "true" || parser.value(keepCompOption) == "1") {
        keep_comp = true;
    }

    fiff_int_t dest_comp = parser.value(destCompsOption).toInt();

    bool pick_all = false;
    if(parser.value(pickAllOption) == "false" || parser.value(pickAllOption) == "0") {
        pick_all = false;
    } else if(parser.value(pickAllOption) == "true" || parser.value(pickAllOption) == "1") {
        pick_all = true;
    }

    // Setup for reading the raw data
    FiffRawData raw(t_fileRaw);
    MNE::setup_compensators(raw, dest_comp, keep_comp);

    RowVectorXi picks;
    if (pick_all) {
        // Pick all
        picks.resize(raw.info.nchan);

        for(qint32 k = 0; k < raw.info.nchan; ++k) {
            picks(k) = k;
        }
    } else {
        QStringList include;
        include << "STI 014";
        bool want_meg   = true;
        bool want_eeg   = false;
        bool want_stim  = false;

        picks = raw.info.pick_types(want_meg,
                                    want_eeg,
                                    want_stim,
                                    include,
                                    raw.info.bads);
    }

    // Read the events
    MatrixXi events;    
    MNE::read_events(t_sEventName,
                     t_fileRawName,
                     events);

    // Read the epochs and reject epochs with EOG higher than 300e-06
    QMap<QString,double> mapReject;
    mapReject.insert("eog", 300e-06);

    MNEEpochDataList data = MNEEpochDataList::readEpochs(raw,
                                                         events,
                                                         fTMin,
                                                         fTMax,
                                                         event,
                                                         mapReject,
                                                         QStringList(),
                                                         picks);

    return a.exec();
}

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================
