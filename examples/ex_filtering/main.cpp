//=============================================================================================================
/**
* @file     main.cpp
* @author   Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     11, 2019
*
* @section  LICENSE
*
* Copyright (C) 2019, Ruben Doerrfel and Matti Hamalainen. All rights reserved.
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
* @brief     Example for filtering of data with realtime filtering. This example is combined with
*            ex_read_write_raw for reading and writing fiff files. So the result of the filtering can be seen
*            in MNE-Browse.
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
#include <rtprocessing/rtfilter.h>

#include <utils/ioutils.h>

//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
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

    MatrixXi picks = raw.info.pick_types(want_meg, want_eeg, want_stim, include, raw.info.bads);
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

    // To read the whole file at once set quantum = to - from + 1;
    fiff_int_t quantum = to - from + 1;

    // Read and write the data
    bool first_buffer = true;

    fiff_int_t first, last;
    MatrixXd data;
    MatrixXd times;

    // initialize filter settings
    QString filter_name =  "example_cosine";
    FilterData::FilterType type = FilterData::BPF;
    double sFreq = raw.info.sfreq;                                          // get Sample freq from Data
    double centerfreq = 10/(sFreq/2.0);                                     // normed nyquist freq.
    double bandwidth = 10/(sFreq/2.0);
    double parkswidth = 1/(sFreq/2.0);

    RtFilter rtFilter;                                                      // filter object
    MatrixXd dataFiltered;                                                  // filter output

    // channel selection - in this case use every channel
    // size = number of channels; value = index channel number
    QVector<int> channelList(raw.info.nchan);
    for (int i = 0; i < raw.info.nchan; i++){
        channelList[i] = i;
    }

    for(first = from; first < to; first+=quantum) {
        last = first+quantum-1;
        if (last > to) {
            last = to;
        }

        if(!raw.read_raw_segment(data,times,first,last/*,picks*/)) {
            printf("error during read_raw_segment\n");
            return -1;
        }

        //Filtering
        printf("Filtering...");
        dataFiltered = rtFilter.filterData(data,type,centerfreq,bandwidth,parkswidth,sFreq,channelList);
        printf("[done]\n");

        //Writing
        printf("Writing...");
        if(first_buffer) {
           if(first > 0) {
               outfid->write_int(FIFF_FIRST_SAMPLE, &first);
           }
           first_buffer = false;
        }

        outfid->write_raw_buffer(dataFiltered,cals);
        printf("[done]\n");
    }

    outfid->finish_writing_raw();

    printf("Finished\n");

    return 0;
}
