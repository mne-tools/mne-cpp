//=============================================================================================================
/**
* @file     main.cpp
* @author   Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     01, 2020
*
* @section  LICENSE
*
* Copyright (C) 2020, Ruben Dörfel and Matti Hamalainen. All rights reserved.
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
* @brief     Example for hpi fitting on raw data with enabled chpi .
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
#include <fiff/fiff_info.h>
#include <fiff/fiff_dig_point_set.h>
#include <inverse/hpiFit/hpifit.h>

//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QFile>
#include <QCommandLineParser>
#include <QDebug>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace FIFFLIB;
using namespace INVERSELIB;

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
    parser.setApplicationDescription("hpiFit Example");
    parser.addHelpOption();

    QCommandLineOption inputOption("fileIn", "The input file <in>.", "in", QCoreApplication::applicationDirPath() + "/MNE-sample-data/raw_meas_stat.fif");

    parser.addOption(inputOption);

    parser.process(a);

    // Init data loading and writing
    QFile t_fileIn(parser.value(inputOption));

    FiffRawData raw(t_fileIn);
    QSharedPointer<FiffInfo> pFiffInfo = QSharedPointer<FIFFLIB::FiffInfo>(new FiffInfo(raw.info));

    RowVectorXd cals;

    // Set up the reading parameters to read the whole file at once
    fiff_int_t from = raw.first_samp;
    fiff_int_t to = raw.last_samp;

    // Only filter MEG and EEG channels
    RowVectorXi picks = raw.info.pick_types(true, false, false);

    // Read, filter and write the data
    MatrixXd matData;
    MatrixXd times;
    qDebug() << "Reading ...\n";
    // Reading
    if(!raw.read_raw_segment(matData, times, from, to)) {
        printf("error during read_raw_segment\n");
        return -1;
    }
    qDebug() << "[done]\n";

    // setup informations to be passed to fitHPI

    QVector<int> vFreqs {154,158,161,166};
    QVector<double> vGof;
    FiffDigPointSet fittedPointSet;
    Eigen::MatrixXd matProjectors = Eigen::MatrixXd::Identity(pFiffInfo->chs.size(), pFiffInfo->chs.size());

    qDebug() << "raw.info sfreq: " << vFreqs;

    // enable debug
    bool bDoDebug = true;
    QString sHPIResourceDir = QCoreApplication::applicationDirPath() + "/HPIFittingDebug";
    qDebug() << "HPI-Fit... \n";
    HPIFit::fitHPI(matData,
                   matProjectors,
                   raw.info.dev_head_t,
                   vFreqs,
                   vGof,
                   fittedPointSet,
                   pFiffInfo,
                   bDoDebug,
                   sHPIResourceDir);

    qDebug() << "[done]\n";
    qDebug() << "\nResults:\n";

    return a.exec();
}
