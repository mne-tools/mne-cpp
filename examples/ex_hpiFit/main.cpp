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
#include <fiff/fiff_cov.h>
#include <inverse/hpiFit/hpifit.h>
#include <inverse/hpiFit/hpifitdata.h>
#include <utils/ioutils.h>
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

void write_pos(const float time, const int index, QSharedPointer<FIFFLIB::FiffInfo> info, Eigen::MatrixXd& position){
    // Write quaternions and time in position matri. Format is the same as in maxfilter .pos files, but we only write quaternions and time. So column 7,8,9 are not used
    QMatrix3x3 rot;

    for(int ir = 0; ir < 3; ir++) {
        for(int ic = 0; ic < 3; ic++) {
            rot(ir,ic) = info->dev_head_t.trans(ir,ic);
        }
    }

    QQuaternion quatHPI = QQuaternion::fromRotationMatrix(rot);
    quatHPI.normalize();
    //std::cout << quatHPI.x() << quatHPI.y() << quatHPI.z() << info->dev_head_t.trans(0,3) << info->dev_head_t.trans(1,3) << info->dev_head_t.trans(2,3) << std::endl;
    position(index,0) = time;
    position(index,1) = quatHPI.x();
    position(index,2) = quatHPI.y();
    position(index,3) = quatHPI.z();
    position(index,4) = info->dev_head_t.trans(0,3);
    position(index,5) = info->dev_head_t.trans(1,3);
    position(index,6) = info->dev_head_t.trans(2,3);
}

int main(int argc, char *argv[])
{
    QElapsedTimer timer;
    QCoreApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("hpiFit Example");
    parser.addHelpOption();

    QCommandLineOption inputOption("fileIn", "The input file <in>.", "in", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/data_with_movement_chpi_raw.fif");

    parser.addOption(inputOption);

    parser.process(a);

    // Init data loading and writing
    QFile t_fileIn(parser.value(inputOption));

    FiffRawData raw(t_fileIn);
    QSharedPointer<FiffInfo> pFiffInfo = QSharedPointer<FIFFLIB::FiffInfo>(new FiffInfo(raw.info));

    RowVectorXd cals;

    // Read Quaternion File
    Eigen::MatrixXd pos;
    UTILSLIB::IOUtils::read_eigen_matrix(pos, QCoreApplication::applicationDirPath() + "/MNE-sample-data/quat_meas_move.txt");

    //std::cout << "quatHPI.x() " << "quatHPI.y() " << "quatHPI.y() " << "trans x " << "trans y " << "trans z " << std::endl;
    Eigen::MatrixXd position = Eigen::MatrixXd::Zero(pos.rows(),pos.cols());
    // Set up the reading parameters to read the whole file at once
    // seconds to read

    // Only filter MEG and EEG channels
    RowVectorXi picks = raw.info.pick_types(true, false, false);

    // Read, filter and write the data
    MatrixXd matData;
    MatrixXd times;

    // Set up the reading parameters
    fiff_int_t from;
    fiff_int_t to;
    fiff_int_t first = raw.first_samp;
    fiff_int_t last = raw.last_samp;

    float quantum_sec = 0.2f;//read and write in 200 ms junks
    fiff_int_t quantum = ceil(quantum_sec*pFiffInfo->sfreq);

    // setup informations for HPI fit
    QVector<int> vFreqs {166,154,161,158};
    QVector<double> vGof;
    FiffDigPointSet fittedPointSet;
    Eigen::MatrixXd matProjectors = Eigen::MatrixXd::Identity(pFiffInfo->chs.size(), pFiffInfo->chs.size());
    bool bDoDebug = true;

    for(int i = 0; i < pos.rows(); i++) {
        from = first + pos(i,0)*pFiffInfo->sfreq;
        to = from + quantum;
        if (to > last) {
            to = last;
        }
        // Reading
        if(!raw.read_raw_segment(matData, times, from, to)) {
            printf("error during read_raw_segment\n");
            return -1;
        }
        qDebug() << "[done]\n";

        QString sHPIResourceDir = QCoreApplication::applicationDirPath() + "/HPIFittingDebug";
        qDebug() << "HPI-Fit...";
        timer.start();
        HPIFit::fitHPI(matData,
                       matProjectors,
                       pFiffInfo->dev_head_t,
                       vFreqs,
                       vGof,
                       fittedPointSet,
                       pFiffInfo,
                       bDoDebug = 0,
                       sHPIResourceDir);
        qDebug() << "The slow operation took" << timer.elapsed() << "milliseconds";
        std::cout << "[done]\n"<< std::endl;
        write_pos(pos(i,0),i,pFiffInfo,position);
    }
    UTILSLIB::IOUtils::write_eigen_matrix(position, QCoreApplication::applicationDirPath() + "/MNE-sample-data/position.txt");
}
