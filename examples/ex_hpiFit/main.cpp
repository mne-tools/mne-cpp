//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>;
 * @version  dev
 * @date     January, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Ruben Dörfel. All rights reserved.
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
#include <utils/mnemath.h>

#include <disp3D/viewers/hpiview.h>

#include <fwd/fwd_coil_set.h>

#include <Eigen/Geometry>

#define _USE_MATH_DEFINES
#include <math.h>

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

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVERSELIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// Member functions
//=============================================================================================================

//=========================================================================================================
    /**
     * Store results from dev_Head_t as quaternions in position matrix. The format is the same as you
     * get from Neuromag's MaxFilter.
     *
     *
     * @param[in]   time          The corresponding time in the measurement for the fit.
     * @param[in]   pFiffInfo     The FiffInfo file from the measurement.
     * @param[out]  position      The matrix to store the results.
     * @param[in]   vGoF          The goodness of fit for each coil.
     * @param[in]   vError        The Hpi estimation Error per coil.
     *
     * ToDo: get estimated movement velocity and stroe it in channel
     */
void storeHeadPosition(float time, const Eigen::Matrix<float, 4,4, Eigen::DontAlign>& devHeadT, MatrixXd& position, const VectorXd& vGoF, const QVector<double>& vError)
{
    // Write quaternions and time in position matrix. Format is the same like MaxFilter's .pos files.
    Matrix3f rot = devHeadT.block(0,0,3,3);

    double error = std::accumulate(vError.begin(), vError.end(), .0) / vError.size();     // HPI estimation Error
    Eigen::Quaternionf quatHPI(rot);

//    qDebug() << "quatHPI.x() " << "quatHPI.y() " << "quatHPI.y() " << "trans x " << "trans y " << "trans z ";
//    qDebug() << quatHPI.x() << quatHPI.y() << quatHPI.z() << devHeadT(0,3) << devHeadT(1,3) << devHeadT(2,3);
    float x = quatHPI.x();
    float y = quatHPI.y();
    float z = quatHPI.z();
    position.conservativeResize(position.rows()+1, 10);
    position(position.rows()-1,0) = time;
    position(position.rows()-1,1) = x;
    position(position.rows()-1,2) = y;
    position(position.rows()-1,3) = z;
    position(position.rows()-1,4) = devHeadT(0,3);
    position(position.rows()-1,5) = devHeadT(1,3);
    position(position.rows()-1,6) = devHeadT(2,3);
    position(position.rows()-1,7) = vGoF.mean();
    position(position.rows()-1,8) = error;
    position(position.rows()-1,9) = 0;
}

//=========================================================================================================
/**
 * Compare new head position with current head position and update dev_head_t if big displacement occured
 *
 * @param[in] devHeadTrans      The device to head transformation matrix to compare to.
 * @param[in] devHeadTransNew   The device to head transformation matrix to be compared.
 * @param[in] treshRot          The threshold for big head rotation in degree
 * @param[in] threshTrans       The threshold for big head movement in m
 *
 * @return state            The status that shows if devHead is updated or not
 *
 */
bool compareTransformation(const Eigen::MatrixX4f& devHeadT, const Eigen::MatrixX4f& devHeadTNew, const float& fThreshRot, const float& fThreshTrans)
{
    bool state = false;

    Matrix3f rot = devHeadT.block(0,0,3,3);
    Matrix3f rotNew = devHeadTNew.block(0,0,3,3);

    VectorXf trans = devHeadT.col(3);
    VectorXf transNew = devHeadTNew.col(3);

    Eigen::Quaternionf quat(rot);
    Eigen::Quaternionf quatNew(rotNew);

    // Compare Rotation
    Eigen::Quaternionf quatCompare;
    float angle;

    // get rotation between both transformations by multiplying with the inverted quaternion
    quatCompare = quat*quatNew.inverse();
    angle = quat.angularDistance(quatNew);
    angle = angle * 180 / M_PI;

    qInfo() << "Eigen angle [degree]: " << angle;

    // Compare translation
    float move = (trans-transNew).norm();
    qInfo() << "Displacement Move [mm]: " << move*1000;

    // compare to thresholds and update
    if(move > fThreshTrans) {
        qInfo() << "Large movement: " << move*1000 << "mm";
        state = true;

    } else if (angle > fThreshRot) {
        qInfo() << "Large rotation: " << angle << "°";
        state = true;

    } else {
        state = false;
    }

    return state;
}

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
    qInstallMessageHandler(UTILSLIB::ApplicationLogger::customLogWriter);
    QElapsedTimer timer;
    QCoreApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("hpiFit Example");
    parser.addHelpOption();
    qInfo() << "Please download the mne-cpp-test-data folder from Github (mne-tools) into mne-cpp/bin.";
    QCommandLineOption inputOption("fileIn", "The input file <in>.", "in", QCoreApplication::applicationDirPath() + "/MNE-sample-data/chpi/raw/sim_move_y_chpi_raw.fif");

    parser.addOption(inputOption);

    parser.process(a);

    // Init data loading and writing
    QFile t_fileIn(parser.value(inputOption));
    FiffRawData raw(t_fileIn);
    QSharedPointer<FiffInfo> pFiffInfo = QSharedPointer<FiffInfo>(new FiffInfo(raw.info));

    // Setup comparison of transformation matrices
    FiffCoordTrans devHeadTrans = pFiffInfo->dev_head_t;    // transformation that only updates after big head movements
    float threshRot = 5;          // in degree
    float threshTrans = 0.005;    // in m

    // Set up the reading parameters
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

//    // create time vector that specifies when to fit
//    int N = ceil((last-first)/quantum);
//    RowVectorXf time = RowVectorXf::LinSpaced(N, 0, N-1) * dT_sec;

    // To fit at specific times outcommend the following block
    // Read Quaternion File
    MatrixXd pos;
    qInfo() << "Specify the path to your position file (.txt)";
    UTILSLIB::IOUtils::read_eigen_matrix(pos, QCoreApplication::applicationDirPath() + "/MNE-sample-data/chpi/pos/posSim_move_y_chpi.txt");
    RowVectorXd time = pos.col(0);

    MatrixXd position;              // Position matrix to save quaternions etc.

    // setup informations for HPI fit (VectorView)
    QVector<int> vFreqs {166,154,161,158};
    QVector<double> vError;
    VectorXd vGoF;
    FiffDigPointSet fittedPointSet;

    // Use SSP + SGM + calibration
    MatrixXd matProjectors = MatrixXd::Identity(pFiffInfo->chs.size(), pFiffInfo->chs.size());

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
    for(int i = 0; i < time.size(); i++) {
        from = first + time(i)*pFiffInfo->sfreq;
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
                       vError,
                       vGoF,
                       fittedPointSet,
                       pFiffInfo,
                       bDoDebug,
                       sHPIResourceDir);
        qInfo() << "The HPI-Fit took" << timer.elapsed() << "milliseconds";
        qInfo() << "[done]";

        HPIFit::storeHeadPosition(time(i), pFiffInfo->dev_head_t.trans, position, vGoF, vError);

        if(UTILSLIB::MNEMath::compareTransformation(devHeadTrans.trans, pFiffInfo->dev_head_t.trans, threshRot, threshTrans)) {
            qInfo() << "Big head displacement: dev_head_t has been updated";
        }

    }
    UTILSLIB::IOUtils::write_eigen_matrix(position, QCoreApplication::applicationDirPath() + "/MNE-sample-data/position.txt");
}
