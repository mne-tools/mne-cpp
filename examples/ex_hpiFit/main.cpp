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
* @brief     Example for cHPI fitting on raw data with SSP. The result is written to a .txt file for comparison with MaxFilter's .pos file.
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
#include <fiff/fiff_cov.h>
#include <inverse/hpiFit/hpifit.h>
#include <inverse/hpiFit/hpifitdata.h>
#include <utils/ioutils.h>
#include <utils/generics/applicationlogger.h>
#include <fwd/fwd_coil_set.h>
//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QFile>
#include <QCommandLineParser>
#include <QDebug>
#include <QGenericMatrix>
#include <QQuaternion>
#include <QElapsedTimer>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVERSELIB;
using namespace FIFFLIB;


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

void write_pos(const float time, QSharedPointer<FIFFLIB::FiffInfo> info, Eigen::MatrixXd& position, const QVector<double>& vGoF){
    // Write quaternions and time in position matrix. Format is the same like MaxFilter's .pos files.
    QMatrix3x3 rot;

    for(int ir = 0; ir < 3; ir++) {
        for(int ic = 0; ic < 3; ic++) {
            rot(ir,ic) = info->dev_head_t.trans(ir,ic);
        }
    }    
    double GoF = std::accumulate(vGoF.begin(), vGoF.end(), .0) / vGoF.size();
    QQuaternion quatHPI = QQuaternion::fromRotationMatrix(rot);
    //std::cout << quatHPI.x() << quatHPI.y() << quatHPI.z() << info->dev_head_t.trans(0,3) << info->dev_head_t.trans(1,3) << info->dev_head_t.trans(2,3) << std::endl;
    position.conservativeResize(position.rows()+1, 10);
    position(position.rows()-1,0) = time;
    position(position.rows()-1,1) = quatHPI.x();
    position(position.rows()-1,2) = quatHPI.y();
    position(position.rows()-1,3) = quatHPI.z();
    position(position.rows()-1,4) = info->dev_head_t.trans(0,3);
    position(position.rows()-1,5) = info->dev_head_t.trans(1,3);
    position(position.rows()-1,6) = info->dev_head_t.trans(2,3);
    position(position.rows()-1,7) = 1-GoF;
    position(position.rows()-1,8) = GoF;
    position(position.rows()-1,9) = 0;
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(UTILSLIB::ApplicationLogger::customLogWriter);
    QElapsedTimer timer;
    QCoreApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("hpiFit Example");
    parser.addHelpOption();
    qInfo() << "Please download the mne-cpp-test-data folder from Github (mne-tools) into mne-cpp/bin.";
    QCommandLineOption inputOption("fileIn", "The input file <in>.", "in", QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/test_hpiFit_raw.fif");

    parser.addOption(inputOption);

    parser.process(a);

    // Init data loading and writing
    QFile t_fileIn(parser.value(inputOption));

    FiffRawData raw(t_fileIn);
    QSharedPointer<FiffInfo> pFiffInfo = QSharedPointer<FIFFLIB::FiffInfo>(new FiffInfo(raw.info));

    //std::cout << "quatHPI.x() " << "quatHPI.y() " << "quatHPI.y() " << "trans x " << "trans y " << "trans z " << std::endl;
    Eigen::MatrixXd position;

    // Set up the reading parameters
    // Only read MEG channels
    RowVectorXi picks = pFiffInfo->pick_types(true, false, false);

    MatrixXd matData;
    MatrixXd times;

    fiff_int_t from;
    fiff_int_t to;
    fiff_int_t first = raw.first_samp;
    fiff_int_t last = raw.last_samp;

    float dT_sec = 0.1;             // time between hpi fits
    float quantum_sec = 0.2f;       // read and write in 200 ms junks
    fiff_int_t quantum = ceil(quantum_sec*pFiffInfo->sfreq);
    fiff_int_t dT = ceil(dT_sec*pFiffInfo->sfreq);
    float t = 0;
    // setup informations for HPI fit (VectorView)
    QVector<int> vFreqs {166,154,161,158};
    QVector<double> vGof;
    FiffDigPointSet fittedPointSet;

    // Use SSP + SGM + calibration
    Eigen::MatrixXd matProjectors = Eigen::MatrixXd::Identity(pFiffInfo->chs.size(), pFiffInfo->chs.size());
    //Do a copy here because we are going to change the activity flags of the SSP's
    FiffInfo infoTemp = *(pFiffInfo.data());
    //Turn on all SSP
    for(int i = 0; i < infoTemp.projs.size(); ++i) {
        infoTemp.projs[i].active = true;
    }
    //Create the projector for all SSP's on
    infoTemp.make_projector(matProjectors);
    //set columns of matrix to zero depending on bad channels indexes
    for(qint32 j = 0; j < infoTemp.bads.size(); ++j) {
        matProjectors.col(infoTemp.ch_names.indexOf(infoTemp.bads.at(j))).setZero();
    }

    // if debugging files are necessary set bDoDebug = true;
    QString sHPIResourceDir = QCoreApplication::applicationDirPath() + "/HPIFittingDebug";
    bool bDoDebug = false;

    // read and fit
    for(from = first; from < last; from+=dT) {
        to = from + quantum;
        if (to > last) {
            to = last;
            qWarning() << "Block size < quantum " << quantum;
        }
        // Reading
        if(!raw.read_raw_segment(matData, times, from, to)) {
            qCritical("error during read_raw_segment");
            return -1;
        }
        qInfo() << "[done]";

        qInfo() << "HPI-Fit...";
        timer.start();
        HPIFit::fitHPI(matData,
                       matProjectors,
                       pFiffInfo->dev_head_t,
                       vFreqs,
                       vGof,
                       fittedPointSet,
                       pFiffInfo,
                       bDoDebug,
                       sHPIResourceDir);
        qInfo() << "The HPI-Fit took" << timer.elapsed() << "milliseconds";
        qInfo() << "[done]";
        write_pos(t,pFiffInfo,position,vGof);
        t +=dT_sec;
    }
    UTILSLIB::IOUtils::write_eigen_matrix(position, QCoreApplication::applicationDirPath() + "/MNE-sample-data/position.txt");
}
