//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>;
 * @since    0.1.0
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/applicationlogger.h>

#include <iostream>
#include <vector>
#include <math.h>

#include <fiff/fiff.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_dig_point_set.h>

#include <inverse/hpiFit/hpifit.h>
#include <inverse/hpiFit/hpifitdata.h>

#include <utils/ioutils.h>
#include <utils/mnemath.h>

#include <fwd/fwd_coil_set.h>

#include <Eigen/Dense>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QFile>
#include <QCommandLineParser>
#include <QDebug>
#include <QtTest>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace INVERSELIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestHpiFit
 *
 * @brief The TestHpiFit class provides hpi fit verifivcation tests
 *
 */
class TestHpiFit: public QObject
{
    Q_OBJECT

public:
    TestHpiFit();

private slots:
    void initTestCase();
    void compareFrequencies();
    void compareTranslation();
    void compareRotation();
    void compareAngle();
    void compareMove();
    void compareDetect();
    void compareTime();
    void cleanupTestCase();

private:
    double dErrorTrans;
    double dErrorQuat;
    double dErrorTime;
    double dErrorAngle;
    double dErrorDetect;
    MatrixXd mRefPos;
    MatrixXd mHpiPos;
    MatrixXd mRefResult;
    MatrixXd mHpiResult;
    QVector<int> vFreqs;
};

//=============================================================================================================

TestHpiFit::TestHpiFit()
: dErrorTrans(0.0003),
dErrorQuat(0.002),
dErrorTime(0.00000001),
dErrorAngle(0.1),
dErrorDetect(0.0)
{
}

//=============================================================================================================

void TestHpiFit::initTestCase()
{
    qInstallMessageHandler(ApplicationLogger::customLogWriter);
    qInfo() << "Error Translation" << dErrorTrans;
    qInfo() << "Error Quaternion" << dErrorQuat;
    QFile t_fileIn(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/test_hpiFit_raw.fif");

    // Make sure test folder exists
    QFileInfo t_fileInInfo(t_fileIn);
    QDir().mkdir(t_fileInInfo.path());

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Read Raw and HPI fit >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    // Setup for reading the raw data
    FiffRawData raw;
    raw = FiffRawData(t_fileIn);
    QSharedPointer<FiffInfo> pFiffInfo = QSharedPointer<FIFFLIB::FiffInfo>(new FiffInfo(raw.info));

    // Only filter MEG channels
    RowVectorXi picks = raw.info.pick_types(true, false, false);
    RowVectorXd cals;

    FiffCoordTrans devHeadT = pFiffInfo->dev_head_t;

    // Set up the reading parameters
    fiff_int_t from;
    fiff_int_t to;
    fiff_int_t first = raw.first_samp;
    fiff_int_t last = raw.last_samp;
    MatrixXd mData, mTimes;

    float quantum_sec = 0.2f;   //read and write in 200 ms junks
    fiff_int_t quantum = ceil(quantum_sec*pFiffInfo->sfreq);

    // Read Quaternion File from maxfilter and calculated movements/rotations with python
    IOUtils::read_eigen_matrix(mRefPos, QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/Result/ref_hpiFit_pos.txt");
    IOUtils::read_eigen_matrix(mRefResult, QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/Result/ref_angle_move.txt");

    mHpiResult = mRefResult;

    // define thresholds for big head movement detection
    float threshRot = 2.0f;
    float threshTrans = 0.002f;

    // Setup informations for HPI fit
    vFreqs = {154,158,161,166};
    QVector<double> vError;
    VectorXd vGoF;
    FiffDigPointSet fittedPointSet;
    Eigen::MatrixXd mProjectors = Eigen::MatrixXd::Identity(pFiffInfo->chs.size(), pFiffInfo->chs.size());
    QString sHPIResourceDir = QCoreApplication::applicationDirPath() + "/HPIFittingDebug";

    HPIFit HPI = HPIFit(pFiffInfo, true);

    // bring frequencies into right order
    from = first + mRefPos(0,0)*pFiffInfo->sfreq;
    to = from + quantum;
    if(!raw.read_raw_segment(mData, mTimes, from, to)) {
        qCritical("error during read_raw_segment");
    }
    qInfo() << "[done]";

    qInfo() << "Order Frequecies: ...";
    HPI.findOrder(mData,
                  mProjectors,
                  pFiffInfo->dev_head_t,
                  vFreqs,
                  vError,
                  vGoF,
                  fittedPointSet,
                  pFiffInfo);
    qInfo() << "[done]";

    for(int i = 0; i < mRefPos.rows(); i++) {
        from = first + mRefPos(i,0)*pFiffInfo->sfreq;
        to = from + quantum;
        if (to > last) {
            to = last;
        }
        qInfo()  << "Reading...";
        if(!raw.read_raw_segment(mData, mTimes, from, to)) {
            qWarning("error during read_raw_segment\n");
        }

        qInfo()  << "HPI-Fit...";

        bool bDoDebug = false;

        HPI.fitHPI(mData,
                   mProjectors,
                   pFiffInfo->dev_head_t,
                   vFreqs,
                   vError,
                   vGoF,
                   fittedPointSet,
                   pFiffInfo,
                   bDoDebug,
                   sHPIResourceDir,
                   200,
                   1e-5f);
        qInfo() << "[done]\n";

        if(MNEMath::compareTransformation(devHeadT.trans, pFiffInfo->dev_head_t.trans, threshRot, threshTrans)) {
            mHpiResult(i,2) = 1;
        }

        HPIFit::storeHeadPosition(mRefPos(i,0), pFiffInfo->dev_head_t.trans, mHpiPos, vGoF, vError);
        mHpiResult(i,0) = devHeadT.translationTo(pFiffInfo->dev_head_t.trans);
        mHpiResult(i,1) = devHeadT.angleTo(pFiffInfo->dev_head_t.trans);

    }
    // For debug: position file for HPIFit
//    UTILSLIB::IOUtils::write_eigen_matrix(mHpiPos, QCoreApplication::applicationDirPath() + "/MNE-sample-data/mHpiPos.txt");
}

//=============================================================================================================

void TestHpiFit::compareFrequencies()
{
    QVector<int> vFreqRef {166, 154, 161, 158};
    QVERIFY(vFreqRef == vFreqs);
}

//=============================================================================================================

void TestHpiFit::compareTranslation()
{
    RowVector3d vDiffTrans;
    vDiffTrans(0) = (mRefPos.col(4)-mHpiPos.col(4)).mean();
    vDiffTrans(1) = (mRefPos.col(5)-mHpiPos.col(5)).mean();
    vDiffTrans(2) = (mRefPos.col(6)-mHpiPos.col(6)).mean();
    qDebug() << "ErrorTrans x: " << std::abs(vDiffTrans(0));
    qDebug() << "ErrorTrans y: " << std::abs(vDiffTrans(1));
    qDebug() << "ErrorTrans z: " << std::abs(vDiffTrans(2));
    QVERIFY(std::abs(vDiffTrans(0)) < dErrorTrans);
    QVERIFY(std::abs(vDiffTrans(1)) < dErrorTrans);
    QVERIFY(std::abs(vDiffTrans(2)) < dErrorTrans);
}

//=============================================================================================================

void TestHpiFit::compareRotation()
{
    RowVector3d vDiffQuat;
    vDiffQuat(0) = (mRefPos.col(1)-mHpiPos.col(1)).mean();
    vDiffQuat(1) = (mRefPos.col(2)-mHpiPos.col(2)).mean();
    vDiffQuat(2) = (mRefPos.col(3)-mHpiPos.col(3)).mean();
    qDebug() << "ErrorQuat q1: " <<std::abs(vDiffQuat(0));
    qDebug() << "ErrorQuat q2: " <<std::abs(vDiffQuat(1));
    qDebug() << "ErrorQuat q3: " <<std::abs(vDiffQuat(2));
    QVERIFY(std::abs(vDiffQuat(0)) < dErrorQuat);
    QVERIFY(std::abs(vDiffQuat(1)) < dErrorQuat);
    QVERIFY(std::abs(vDiffQuat(2)) < dErrorQuat);
}

//=============================================================================================================

void TestHpiFit::compareMove()
{
    float fDiffMove = (mRefResult.col(0)-mHpiResult.col(0)).mean();
    fDiffMove = std::abs(fDiffMove);

    qDebug() << "DiffMove: [m]" << fDiffMove;
    QVERIFY(std::abs(fDiffMove) < dErrorTrans);
}

//=============================================================================================================

void TestHpiFit::compareAngle()
{
    float fDiffAngle = (mRefResult.col(1)-mRefResult.col(1)).mean();
    fDiffAngle = std::abs(fDiffAngle);

    qDebug() << "DiffAngle: [degree]" << fDiffAngle;
    QVERIFY(std::abs(fDiffAngle) < dErrorAngle);
}

//=============================================================================================================

void TestHpiFit::compareDetect()
{
    float fDiffCompare = (mRefResult.col(2)-mRefResult.col(2)).mean();
    fDiffCompare = std::abs(fDiffCompare);

    qDebug() << "DiffCompare: " << fDiffCompare;
    QVERIFY(std::abs(fDiffCompare) == dErrorDetect);
}

//=============================================================================================================

void TestHpiFit::compareTime()
{
    MatrixXd mDiff = MatrixXd::Zero(mRefPos.rows(),1);
    mDiff.col(0) = mRefPos.col(0)-mHpiPos.col(0);
    float fDiffTime = mDiff.col(0).mean();

    qDebug() << "ErrorTime: " << fDiffTime;
    QVERIFY(std::abs(fDiffTime) < dErrorTime);
}

//=============================================================================================================

void TestHpiFit::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestHpiFit)
#include "test_hpiFit.moc"
