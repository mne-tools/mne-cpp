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
 * @brief    Example of processing raw data (read and write)
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <math.h>

#include <fiff/fiff.h>
#include <utils/generics/applicationlogger.h>

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
    parser.setApplicationDescription("Read Write Raw Example");
    parser.addHelpOption();

    QCommandLineOption inputOption("fileIn", "The input file <in>.", "in", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis_raw.fif");
    QCommandLineOption outputOption("fileOut", "The output file <out>.", "out", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/test_output.fif");

    parser.addOption(inputOption);
    parser.addOption(outputOption);

    parser.process(a);

    // Init data loading and writing
    QFile t_fileIn(parser.value(inputOption));
    QFile t_fileOut(parser.value(outputOption));

    FiffRawData raw(t_fileIn);

    // Set up pick list: MEG + STI 014 - bad channels
    bool want_meg   = true;
    bool want_eeg   = true;
    bool want_stim  = true;
    QStringList include;
    include << "STI 014";

    RowVectorXi picks = raw.info.pick_types(want_meg, want_eeg, want_stim, include, raw.info.bads);
    if(picks.cols() == 0) {
        include.clear();
        include << "STI101" << "STI201" << "STI301";
        picks = raw.info.pick_types(want_meg, want_eeg, want_stim, include, raw.info.bads);
        if(picks.cols() == 0) {
            printf("channel list may need modification\n");
            return -1;
        }
    }

    RowVectorXd cals;
    FiffStream::SPtr outfid = FiffStream::start_writing_raw(t_fileOut, raw.info, cals/*, picks*/);

    // Set up the reading parameters
    fiff_int_t from = raw.first_samp;
    fiff_int_t to = raw.last_samp;
    //fiff_int_t to = raw.first_samp + raw.info.sfreq * 6;
    float quantum_sec = 10.0f;//read and write in 10 sec junks
    fiff_int_t quantum = ceil(quantum_sec*raw.info.sfreq);

    // To read the whole file at once set quantum = to - from + 1;
    // Read and write the data
    bool first_buffer = true;

    fiff_int_t first, last;
    MatrixXd data;
    MatrixXd times;

    for(first = from; first < to; first+=quantum) {
        last = first+quantum-1;
        if (last > to) {
            last = to;
        }

        if(!raw.read_raw_segment(data,times,first,last/*,picks*/)) {
            printf("error during read_raw_segment\n");
            return -1;
        }

        // You can add your own miracle here
        printf("Writing...");
        if(first_buffer) {
           if(first > 0) {
               outfid->write_int(FIFF_FIRST_SAMPLE, &first);
           }
           first_buffer = false;
        }

        outfid->write_raw_buffer(data,cals);
        printf("[done]\n");
    }

    outfid->finish_writing_raw();

    printf("Finished\n");

    return 0;
}
