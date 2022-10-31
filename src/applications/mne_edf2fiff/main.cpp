//=============================================================================================================
/**
* @file     main.cpp
* @author   Simon Heinke <simon.heinke@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     April, 2019
*
* @section  LICENSE
*
* Copyright (C) 2019, Simon Heinke and Matti Hamalainen. All rights reserved.
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
* @brief    Converting EDF data into Fiff data.T
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <algorithm>
#include <vector>

#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_raw_data.h>

#include "edf_info.h"
#include "edf_raw_data.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QFile>
#include <QCommandLineParser>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace EDF2FIFF;
using namespace FIFFLIB;
using namespace Eigen;

//*************************************************************************************************************
//=============================================================================================================
// MAIN
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

    // command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription("EDF to Fiff conversion. Variable channel frequencies are supported. Interrupted recordings are not supported.");
    parser.addHelpOption();

    QCommandLineOption inputOption("fileIn", "The input file. Needs to be specified.", "in", "");
    QCommandLineOption outputOption("fileOut", "The output file. If not specified, this will be the same filename as the input file.", "out", "");
    QCommandLineOption scaleOption("scaleFactor", "The raw value scaling factor. Must be a float number. If not specified, this will be 1e6.", "in", "1e6");

    parser.addOption(inputOption);
    parser.addOption(outputOption);
    parser.addOption(scaleOption);

    parser.process(a);

    QString sInputFile = parser.value(inputOption);
    QString sOutputFile = parser.value(outputOption);
    QString sScaleFactor = parser.value(scaleOption);

    // check for correct usage:
    if(sInputFile.isEmpty()) {
        parser.showHelp(0);
    }
    if(!sInputFile.toUpper().endsWith(".EDF")) {
        qDebug() << "Not an EDF file: " << sInputFile;
        return 0;
    }

    if(sScaleFactor.toFloat() == 0.0f) {
        qDebug() << "Not a float number: " << sScaleFactor;
        return 0;
    }

    // if the user did not specify an output file, simply use the same location as the input file:
    if(sOutputFile.isEmpty()) {
        qDebug() << "No output file specified, using same filename for output FIFF file";
        sOutputFile = sInputFile.left(sInputFile.size() - 3);  // cut the 'edf'
        sOutputFile = sOutputFile.append("fif"); // append 'fif'
    }

    // init data loading and writing
    QFile t_fileIn(sInputFile);
    QFile t_fileOut(sOutputFile);

    // initialize raw data
    EDFRawData edfRaw(&t_fileIn, sScaleFactor.toFloat());
    // print basic info
    EDFInfo edfInfo = edfRaw.getInfo();
    // qDebug().noquote() << edfInfo.getAsString();

    // convert to fiff
    FiffRawData fiffRaw = edfRaw.toFiffRawData();

    // set up the reading parameters
    float fTimesliceSeconds = 10.0f; //read and write in 10 sec chunks
    int iTimesliceSamples = static_cast<int>(ceil(fTimesliceSeconds * fiffRaw.info.sfreq));

    RowVectorXd cals;
    FiffStream::SPtr outfid = FiffStream::start_writing_raw(t_fileOut, fiffRaw.info, cals);

    // copied from read/write example
    fiff_int_t first = 0;  // EDF files start at index 0
    outfid->write_int(FIFF_FIRST_SAMPLE, &first);

    // read chunks, remember how many samples were already read
    int iSamplesRead = 0;

    while(iSamplesRead < edfInfo.getSampleCount()) {
        int iNextChunkSize = std::min(iTimesliceSamples, edfInfo.getSampleCount() - iSamplesRead);
        // EDF sample indexing starts at 0, simply use samplesRead as argument to read_raw_segment
        MatrixXd data = edfRaw.read_raw_segment(iSamplesRead, iSamplesRead + iNextChunkSize).cast<double>();

        iSamplesRead += iNextChunkSize;

        outfid->write_raw_buffer(data, cals);
    }

    outfid->finish_writing_raw();
    qDebug() << "Writing finished !";

    return 0;
}
