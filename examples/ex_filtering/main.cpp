//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Ruben Doerfel <Ruben.Doerfel@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     11, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Ruben Doerfel, Lorenz Esch. All rights reserved.
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
 * @brief     Example for filtering data with a user defined FIR filter and writing the result to a file.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <math.h>

#include <fiff/fiff.h>

#include <utils/filterTools/filterdata.h>
#include <utils/generics/applicationlogger.h>

#include <rtprocessing/rtfilter.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QFile>
#include <QCommandLineParser>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace RTPROCESSINGLIB;
using namespace Eigen;


//*************************************************************************************************************
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
    qInstallMessageHandler(ApplicationLogger::customLogWriter);
    QCoreApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Read Write Raw Example");
    parser.addHelpOption();

    QCommandLineOption inputOption("fileIn", "The input file <in>.", "in", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis_raw.fif");
    QCommandLineOption outputOption("fileOut", "The output file <out>.", "out", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis_filt_raw.fif");

    parser.addOption(inputOption);
    parser.addOption(outputOption);

    parser.process(a);

    // Init data loading and writing
    QFile t_fileIn(parser.value(inputOption));
    QFile t_fileOut(parser.value(outputOption));

    FiffRawData raw(t_fileIn);

    RowVectorXd cals;
    FiffStream::SPtr outfid = FiffStream::start_writing_raw(t_fileOut, raw.info, cals);

    // Set up the reading parameters to read the whole file at once
    fiff_int_t from = raw.first_samp;
    fiff_int_t to = raw.last_samp;

    // Initialize filter settings
    QString filter_name =  "Cosine_BPF";
    FilterData::FilterType type = FilterData::BPF;
    double sFreq = raw.info.sfreq;
    double dCenterfreq = 10;
    double dBandwidth = 10;
    double dTransition = 1;

    RtFilter rtFilter;
    MatrixXd dataFiltered;

    // Only filter MEG and EEG channels
    RowVectorXi picks = raw.info.pick_types(true, false, false);

    // Read, filter and write the data
    MatrixXd data;
    MatrixXd times;

    // Reading
    if(!raw.read_raw_segment(data, times, from, to)) {
        printf("error during read_raw_segment\n");
        return -1;
    }

    // Filtering
    printf("Filtering...");
    dataFiltered = rtFilter.filterData(data,
                                       type,
                                       dCenterfreq,
                                       dBandwidth,
                                       dTransition,
                                       sFreq,
                                       picks);
    printf("[done]\n");

    // Writing
    printf("Writing...");
    outfid->write_int(FIFF_FIRST_SAMPLE, &from);
    outfid->write_raw_buffer(dataFiltered,cals);
    printf("[done]\n");

    outfid->finish_writing_raw();

    return 0;
}
