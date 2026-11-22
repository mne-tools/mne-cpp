//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     test_hpiFit_integration.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.0
 * @date     February, 2020
 * @brief     Integration tests for the HPI fitting
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>
#include <math.h>
#include <mne/fiff/fiff.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_dig_point_set.h>
#include <mne/inv/hpi/inv_hpi_fit.h>
#include <mne/inv/hpi/inv_hpi_data_updater.h>
#include <mne/math/matrix_io.h>
#include <mne/fwd/fwd_coil_set.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QFile>
#include <QCommandLineParser>
#include <QtTest>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <iostream>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestHpiFitIntegration
 *
 * @brief The TestHpiFitIntegration class provides hpi fit verifivcation tests
 *
 */
class TestHpiFitIntegration : public QObject
{
    Q_OBJECT

public:
    TestHpiFitIntegration();

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
    std::vector<int> vFreqs;
};

//=============================================================================================================

TestHpiFitIntegration::TestHpiFitIntegration()
: dErrorTrans(0.0003)
, dErrorQuat(0.002)
, dErrorTime(0.00000001)
, dErrorAngle(0.1)
, dErrorDetect(0.0)
{
}

//=============================================================================================================

void TestHpiFitIntegration::initTestCase()
{
    Log::info("Error Translation {}", dErrorTrans);
    Log::info("Error Quaternion {}", dErrorQuat);
    QFile t_fileIn(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/test_hpiFit_raw.fif");

    // Make sure test folder exists
    QFileInfo t_fileInInfo(t_fileIn);
    QDir().mkdir(t_fileInInfo.path());

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Read Raw and HPI fit >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    // Setup for reading the raw data
    FiffRawData raw;
    raw = FiffRawData((QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/test_hpiFit_raw.fif").toStdString());
    std::shared_ptr<FiffInfo> pFiffInfo = std::make_shared<FiffInfo>(raw.info);

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

    float quantum_sec = 0.2f; //read and write in 200 ms junks
    fiff_int_t quantum = ceil(quantum_sec * pFiffInfo->sfreq);

    // Read Quaternion File from maxfilter and calculated movements/rotations with python
    mne::read_matrix(mRefPos, (QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/ref_hpiFit_pos.txt").toStdString());
    mne::read_matrix(mRefResult, (QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/ref_angle_move.txt").toStdString());

    mHpiResult = mRefResult;

    // define thresholds for big head movement detection
    float threshRot = 2.0f;
    float threshTrans = 0.002f;

    // Setup informations for HPI fit
    vFreqs = {154, 158, 161, 166};
    Eigen::MatrixXd mProjectors = Eigen::MatrixXd::Identity(pFiffInfo->chs.size(), pFiffInfo->chs.size());
    QString sHPIResourceDir = QCoreApplication::applicationDirPath() + "/HPIFittingDebug";

    InvHpiDataUpdater hpiDataUpdater = InvHpiDataUpdater(pFiffInfo);
    InvHpiFit HPI = InvHpiFit(hpiDataUpdater.getSensors());
    int iSampleFreq = pFiffInfo->sfreq;
    int iLineFreq = pFiffInfo->linefreq;
    InvHpiModelParameters hpiModelParameters(vFreqs,
                                             iSampleFreq,
                                             iLineFreq,
                                             true);

    from = first + mRefPos(0, 0) * pFiffInfo->sfreq;
    to = from + quantum;
    if (!raw.read_raw_segment(mData, mTimes, from, to)) {
        Log::critical("error during read_raw_segment");
    }

    Log::info("Order Frequecies: ...");
    hpiDataUpdater.prepareDataAndProjectors(mData, mProjectors);
    const auto& matProjectedData = hpiDataUpdater.getProjectedData();
    const auto& matPreparedProjectors = hpiDataUpdater.getProjectors();
    const auto& matCoilsHead = hpiDataUpdater.getHpiDigitizer();

    HpiFitResult hpiFitResult;
    HPI.fit(matProjectedData, matPreparedProjectors, hpiModelParameters, matCoilsHead, true, hpiFitResult);
    hpiModelParameters = InvHpiModelParameters(hpiFitResult.hpiFreqs,
                                               pFiffInfo->sfreq,
                                               pFiffInfo->linefreq,
                                               true);
    vFreqs = hpiFitResult.hpiFreqs;

    Log::info("[done]");

    for (int i = 0; i < mRefPos.rows(); i++) {
        from = first + mRefPos(i, 0) * pFiffInfo->sfreq;
        to = from + quantum;
        if (to > last) {
            to = last;
        }

        if (!raw.read_raw_segment(mData, mTimes, from, to)) {
            Log::warning("error during read_raw_segment\n");
        }

        hpiDataUpdater.prepareDataAndProjectors(mData, mProjectors);
        const auto& matProjectedDataLoop = hpiDataUpdater.getProjectedData();
        const auto& matPreparedProjectorsLoop = hpiDataUpdater.getProjectors();
        HPI.fit(matProjectedDataLoop,
                matPreparedProjectorsLoop,
                hpiModelParameters,
                matCoilsHead,
                hpiFitResult);

        if (InvHpiFit::compareTransformation(devHeadT.trans, hpiFitResult.devHeadTrans.trans, threshRot, threshTrans)) {
            mHpiResult(i, 2) = 1;
        }

        InvHpiFit::storeHeadPosition(mRefPos(i, 0), hpiFitResult.devHeadTrans.trans, mHpiPos, hpiFitResult.GoF, hpiFitResult.errorDistances);
        mHpiResult(i, 0) = devHeadT.translation_to(hpiFitResult.devHeadTrans.trans);
        mHpiResult(i, 1) = devHeadT.angle_to(hpiFitResult.devHeadTrans.trans);
    }
    // For debug: position file for InvHpiFit
    //    mne::write_matrix(mHpiPos, QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/mHpiPos.txt");
}

//=============================================================================================================

void TestHpiFitIntegration::compareFrequencies()
{
    std::vector<int> vFreqRef{166, 154, 161, 158};
    QVERIFY(vFreqRef == vFreqs);
}

//=============================================================================================================

void TestHpiFitIntegration::compareTranslation()
{
    RowVector3d vDiffTrans;
    vDiffTrans(0) = (mRefPos.col(4) - mHpiPos.col(4)).mean();
    vDiffTrans(1) = (mRefPos.col(5) - mHpiPos.col(5)).mean();
    vDiffTrans(2) = (mRefPos.col(6) - mHpiPos.col(6)).mean();
    Log::debug("ErrorTrans x: {}", std::abs(vDiffTrans(0)));
    Log::debug("ErrorTrans y: {}", std::abs(vDiffTrans(1)));
    Log::debug("ErrorTrans z: {}", std::abs(vDiffTrans(2)));
    QVERIFY(std::abs(vDiffTrans(0)) < dErrorTrans);
    QVERIFY(std::abs(vDiffTrans(1)) < dErrorTrans);
    QVERIFY(std::abs(vDiffTrans(2)) < dErrorTrans);
}

//=============================================================================================================

void TestHpiFitIntegration::compareRotation()
{
    RowVector3d vDiffQuat;
    vDiffQuat(0) = (mRefPos.col(1) - mHpiPos.col(1)).mean();
    vDiffQuat(1) = (mRefPos.col(2) - mHpiPos.col(2)).mean();
    vDiffQuat(2) = (mRefPos.col(3) - mHpiPos.col(3)).mean();
    Log::debug("ErrorQuat q1: {}", std::abs(vDiffQuat(0)));
    Log::debug("ErrorQuat q2: {}", std::abs(vDiffQuat(1)));
    Log::debug("ErrorQuat q3: {}", std::abs(vDiffQuat(2)));
    QVERIFY(std::abs(vDiffQuat(0)) < dErrorQuat);
    QVERIFY(std::abs(vDiffQuat(1)) < dErrorQuat);
    QVERIFY(std::abs(vDiffQuat(2)) < dErrorQuat);
}

//=============================================================================================================

void TestHpiFitIntegration::compareMove()
{
    float fDiffMove = (mRefResult.col(0) - mHpiResult.col(0)).mean();
    fDiffMove = std::abs(fDiffMove);

    Log::debug("DiffMove: [m] {}", fDiffMove);
    QVERIFY(std::abs(fDiffMove) < dErrorTrans);
}

//=============================================================================================================

void TestHpiFitIntegration::compareAngle()
{
    float fDiffAngle = (mRefResult.col(1) - mHpiResult.col(1)).mean();
    fDiffAngle = std::abs(fDiffAngle);

    Log::debug("DiffAngle: [degree] {}", fDiffAngle);
    QVERIFY(std::abs(fDiffAngle) < dErrorAngle);
}

//=============================================================================================================

void TestHpiFitIntegration::compareDetect()
{
    float fDiffCompare = (mRefResult.col(2) - mHpiResult.col(2)).mean();
    fDiffCompare = std::abs(fDiffCompare);

    Log::debug("DiffCompare: {}", fDiffCompare);
    QVERIFY(std::abs(fDiffCompare) == dErrorDetect);
}

//=============================================================================================================

void TestHpiFitIntegration::compareTime()
{
    MatrixXd mDiff = MatrixXd::Zero(mRefPos.rows(), 1);
    mDiff.col(0) = mRefPos.col(0) - mHpiPos.col(0);
    float fDiffTime = mDiff.col(0).mean();

    Log::debug("ErrorTime: {}", fDiffTime);
    QVERIFY(std::abs(fDiffTime) < dErrorTime);
}

//=============================================================================================================

void TestHpiFitIntegration::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestHpiFitIntegration)
#include "test_hpiFit_integration.moc"
