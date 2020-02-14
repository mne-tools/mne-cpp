//=============================================================================================================
/**
* @file     main.cpp
* @author   Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>;
* @version  dev
* @date     February, 2020
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
* @brief     Test for fitHpi function in inverse library.
*
*/

///*************************************************************************************************************
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

#include <Eigen/Dense>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QFile>
#include <QCommandLineParser>
#include <QDebug>
#include <QQuaternion>
#include <QtTest>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace INVERSELIB;
using namespace Eigen;
//=============================================================================================================
/**
 * DECLARE CLASS TestFiffRFR
 *
 * @brief The TestFitHPI class provides hpi fit verifivcation tests
 *
 */
class TestFitHPI: public QObject
{
    Q_OBJECT

public:
    TestFitHPI();

private slots:
    void initTestCase();
    void compareTranslation();
    void compareRotation();
    void compareTime();
    void cleanupTestCase();

private:
    double errorTrans = 0.0003;
    double errorQuat = 0.002;
    double errorTime = 0.00000001;
    MatrixXd ref_pos;
    MatrixXd hpi_pos;
};

//*************************************************************************************************************

TestFitHPI::TestFitHPI()
{
}

//*************************************************************************************************************

void write_pos(const float time, const int index, QSharedPointer<FIFFLIB::FiffInfo> info, Eigen::MatrixXd& position){
    // Write quaternions and time in position matrix. Format is the same as in maxfilter .pos files, but we only write quaternions and time. So column 7,8,9 are not used
    QMatrix3x3 rot;

    for(int ir = 0; ir < 3; ir++) {
        for(int ic = 0; ic < 3; ic++) {
            rot(ir,ic) = info->dev_head_t.trans(ir,ic);
        }
    }

    QQuaternion quatHPI = QQuaternion::fromRotationMatrix(rot);
    //std::cout << quatHPI.x() << quatHPI.y() << quatHPI.z() << info->dev_head_t.trans(0,3) << info->dev_head_t.trans(1,3) << info->dev_head_t.trans(2,3) << std::endl;
    position(index,0) = time;
    position(index,1) = quatHPI.x();
    position(index,2) = quatHPI.y();
    position(index,3) = quatHPI.z();
    position(index,4) = info->dev_head_t.trans(0,3);
    position(index,5) = info->dev_head_t.trans(1,3);
    position(index,6) = info->dev_head_t.trans(2,3);
}

void TestFitHPI::initTestCase()
{
    qInstallMessageHandler(UTILSLIB::ApplicationLogger::customLogWriter);
    qInfo() << "Error Translation" << errorTrans;
    qInfo() << "Error Quaternion" << errorQuat;
    QFile t_fileIn(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/test_hpiFit_raw.fif");

    // Make sure test folder exists
    QFileInfo t_fileInInfo(t_fileIn);
    QDir().mkdir(t_fileInInfo.path());

    //*********************************************************************************************************
    // Read and HPI fit
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Read and HPI fit  >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    // Setup for reading the raw data
    FiffRawData raw;
    raw = FiffRawData(t_fileIn);
    QSharedPointer<FiffInfo> pFiffInfo = QSharedPointer<FIFFLIB::FiffInfo>(new FiffInfo(raw.info));

    // Only filter MEG channels
    RowVectorXi picks = raw.info.pick_types(true, false, false);
    RowVectorXd cals;

    // Set up the reading parameters
    fiff_int_t from;
    fiff_int_t to;
    fiff_int_t first = raw.first_samp;
    fiff_int_t last = raw.last_samp;
    MatrixXd matData, times;

    float quantum_sec = 0.2f;   //read and write in 200 ms junks
    fiff_int_t quantum = ceil(quantum_sec*pFiffInfo->sfreq);

    // Read Quaternion File from maxfilter
    UTILSLIB::IOUtils::read_eigen_matrix(ref_pos, QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/Result/ref_hpiFit_pos.txt");
    hpi_pos = Eigen::MatrixXd::Zero(ref_pos.rows(),ref_pos.cols());

    // Setup informations for HPI fit
    QVector<int> vFreqs {166,154,161,158};
    QVector<double> vGof;
    FiffDigPointSet fittedPointSet;
    Eigen::MatrixXd matProjectors = Eigen::MatrixXd::Identity(pFiffInfo->chs.size(), pFiffInfo->chs.size());
    QString sHPIResourceDir = QCoreApplication::applicationDirPath() + "/HPIFittingDebug";
    bool bDoDebug = false;

    for(int i = 0; i < ref_pos.rows(); i++) {
        from = first + ref_pos(i,0)*pFiffInfo->sfreq;
        to = from + quantum;
        if (to > last) {
            to = last;
        }
        qInfo()  << "Reading...";
        if(!raw.read_raw_segment(matData, times, from, to)) {
            qWarning("error during read_raw_segment\n");
        }

        qInfo()  << "HPI-Fit...";
        HPIFit::fitHPI(matData,
                       matProjectors,
                       pFiffInfo->dev_head_t,
                       vFreqs,
                       vGof,
                       fittedPointSet,
                       pFiffInfo,
                       bDoDebug = 0,
                       sHPIResourceDir);
        qInfo() << "[done]\n";
        write_pos(ref_pos(i,0),i,pFiffInfo,hpi_pos);
    }
    // For debug: position file for HPIFit
    // UTILSLIB::IOUtils::write_eigen_matrix(hpi_pos, QCoreApplication::applicationDirPath() + "/MNE-sample-data/hpi_pos.txt");
}

//*************************************************************************************************************

void TestFitHPI::compareTranslation()
{
    MatrixXd diff = MatrixXd::Zero(ref_pos.rows(),3);
    RowVector3d diff_trans;
    diff_trans(0) = (ref_pos.col(4)-hpi_pos.col(4)).mean();
    diff_trans(1) = (ref_pos.col(5)-hpi_pos.col(5)).mean();
    diff_trans(2) = (ref_pos.col(6)-hpi_pos.col(6)).mean();
    qDebug() << "errorTrans x: " << std::abs(diff_trans(0));
    qDebug() << "errorTrans y: " << std::abs(diff_trans(1));
    qDebug() << "errorTrans z: " << std::abs(diff_trans(2));
    QVERIFY(std::abs(diff_trans(0)) < errorTrans);
    QVERIFY(std::abs(diff_trans(1)) < errorTrans);
    QVERIFY(std::abs(diff_trans(2)) < errorTrans);
}

//*************************************************************************************************************
void TestFitHPI::compareRotation()
{
    MatrixXd diff = MatrixXd::Zero(ref_pos.rows(),3);
    RowVector3d diff_quat;
    diff_quat(0) = (ref_pos.col(1)-hpi_pos.col(1)).mean();
    diff_quat(1) = (ref_pos.col(2)-hpi_pos.col(2)).mean();
    diff_quat(2) = (ref_pos.col(3)-hpi_pos.col(3)).mean();
    qDebug() << "errorQuat q1: " <<std::abs(diff_quat(0));
    qDebug() << "errorQuat q2: " <<std::abs(diff_quat(1));
    qDebug() << "errorQuat q3: " <<std::abs(diff_quat(2));
    QVERIFY(std::abs(diff_quat(0)) < errorQuat);
    QVERIFY(std::abs(diff_quat(1)) < errorQuat);
    QVERIFY(std::abs(diff_quat(2)) < errorQuat);
}

//*************************************************************************************************************
void TestFitHPI::compareTime()
{
    MatrixXd diff = MatrixXd::Zero(ref_pos.rows(),1);
    diff.col(0) = ref_pos.col(0)-hpi_pos.col(0);
    float diff_t = diff.col(0).mean();
    qDebug() << "errorTime: " << diff_t;
    QVERIFY(std::abs(diff_t) < errorTime);
}

//*************************************************************************************************************

void TestFitHPI::cleanupTestCase()
{
}

//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFitHPI)
#include "test_hpiFit.moc"
