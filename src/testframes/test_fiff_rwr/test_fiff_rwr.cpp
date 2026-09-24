//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *
 * @file     test_fiff_rwr.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     December, 2015
 * @brief    Test for reading writing reading a fiff raw file
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/mne_logger.h>

#include <fiff/fiff.h>

#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QTemporaryDir>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestFiffRWR
 *
 * @brief The TestFiffRWR class provides read write read fiff verification tests
 *
 */
class TestFiffRWR: public QObject
{
    Q_OBJECT

public:
    TestFiffRWR();

private slots:
    void initTestCase();
    void compareData();
    void compareTimes();
    void compareInfo();
    void cleanupTestCase();

private:
    double dEpsilon;
    QTemporaryDir m_tempDir;

    FiffRawData rawFirstInRaw;

    MatrixXd mFirstInData;
    MatrixXd mFirstInTimes;

    FiffRawData rawSecondInRaw;

    MatrixXd mSecondInData;
    MatrixXd mSecondInTimes;
};

//=============================================================================================================

TestFiffRWR::TestFiffRWR()
: dEpsilon(0.000001)
{
}

//=============================================================================================================

void TestFiffRWR::initTestCase()
{
    qInstallMessageHandler(UTILSLIB::MNELogger::customLogWriter);
    qDebug() << "dEpsilon" << dEpsilon;

    QFile t_fileIn(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif");
    QVERIFY(m_tempDir.isValid());
    QFile t_fileOut(m_tempDir.filePath("sample_audvis_trunc_raw_test_rwr_out.fif"));

    //*********************************************************************************************************
    // First Read & Write
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Read & Write >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    //
    //   Setup for reading the raw data
    //
    rawFirstInRaw = FiffRawData(t_fileIn);

    //
    //   Set up pick list: MEG + STI 014 - bad channels
    //
    //
    bool bWantMeg   = true;
    bool bWantEeg   = false;
    bool bWantStim  = false;
    QStringList include;
    include << "STI 014";

    MatrixXi vPicks = rawFirstInRaw.info.pick_types(bWantMeg, bWantEeg, bWantStim, include, rawFirstInRaw.info.bads); // prefer member function
    if(vPicks.cols() == 0)
    {
        include.clear();
        include << "STI101" << "STI201" << "STI301";
        vPicks = rawFirstInRaw.info.pick_types(bWantMeg, bWantEeg, bWantStim, include, rawFirstInRaw.info.bads);// prefer member function
        if(vPicks.cols() == 0)
        {
            printf("channel list may need modification\n");
        }
    }
    //
    RowVectorXd vCals;

    FiffStream::SPtr outfid = FiffStream::start_writing_raw(t_fileOut,rawFirstInRaw.info, vCals/*, vPicks*/);

    //
    //   Set up the reading parameters
    //
    float fQuantumSec = 1.0f;//read and write in 1 sec junks
    fiff_int_t from = rawFirstInRaw.first_samp;
    fiff_int_t to = rawFirstInRaw.last_samp;
    fiff_int_t quantum = ceil(fQuantumSec*rawFirstInRaw.info.sfreq);

    //
    //   To read the whole file at once set
    //
    //quantum     = to - from + 1;
    //
    //
    //   Read and write all the data
    //
    bool bFirstBuffer = true;

    fiff_int_t first, last;

    for(first = from; first < to; first+=quantum)
    {
        last = first+quantum-1;
        if (last > to)
        {
            last = to;
        }

        if (!rawFirstInRaw.read_raw_segment(mFirstInData, mFirstInTimes, first, last/*,vPicks*/))
        {
                printf("error during read_raw_segment\n");
        }
        //
        //   You can add your own miracle here
        //
        printf("Writing...");
        if (bFirstBuffer)
        {
           if (first > 0)
               outfid->write_int(FIFF_FIRST_SAMPLE,&first);
           bFirstBuffer = false;
        }
        outfid->write_raw_buffer(mFirstInData, vCals);
        printf("[done]\n");
    }

    outfid->finish_writing_raw();

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Read & Write Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");

    //*********************************************************************************************************
    // Second Read
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Read Again >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    //
    //   Setup for reading the raw data from out file
    //
    rawSecondInRaw = FiffRawData(t_fileOut);

    //
    //   Set up pick list: MEG + STI 014 - bad channels
    //
    //

    vPicks = rawSecondInRaw.info.pick_types(bWantMeg, bWantEeg, bWantStim, include, rawSecondInRaw.info.bads); // prefer member function
    if(vPicks.cols() == 0)
    {
        include.clear();
        include << "STI101" << "STI201" << "STI301";
        vPicks = rawSecondInRaw.info.pick_types(bWantMeg, bWantEeg, bWantStim, include, rawSecondInRaw.info.bads);// prefer member function
        if(vPicks.cols() == 0)
        {
            printf("channel list may need modification\n");
        }
    }

    //
    //   Read all the data
    //

    for(first = from; first < to; first+=quantum)
    {
        last = first+quantum-1;
        if (last > to)
        {
            last = to;
        }

        if (!rawSecondInRaw.read_raw_segment(mSecondInData,mSecondInTimes,first,last/*,vPicks*/))
        {
                printf("error during read_raw_segment\n");
        }
    }

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Read Again Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//=============================================================================================================

void TestFiffRWR::compareData()
{
    MatrixXd mDataDiff = mFirstInData - mSecondInData;
//    std::cout << "\tCompare data:\n";
//    std::cout << "\tFirst data\n" << mFirstInData.block(0,0,4,4) << "\n";
//    std::cout << "\tSecond data\n" << mSecondInData.block(0,0,4,4) << "\n";

    QVERIFY( mDataDiff.sum() < dEpsilon );
}

//=============================================================================================================

void TestFiffRWR::compareTimes()
{
    MatrixXd mTimesDiff = mFirstInTimes - mSecondInTimes;
//    std::cout << "\tCompare Times:\n";
//    std::cout << "\tFirst times\n" << mFirstInTimes.block(0,0,1,4) << "\n";
//    std::cout << "\tSecond times\n" << mSecondInTimes.block(0,0,1,4) << "\n";

    QVERIFY( mTimesDiff.sum() < dEpsilon );
}

//=============================================================================================================

void TestFiffRWR::compareInfo()
{
    //Sampling frequency
    std::cout << "[1] Sampling Frequency Check\n";
    QVERIFY( rawFirstInRaw.info.sfreq == rawSecondInRaw.info.sfreq );

    //Projection
    std::cout << "[2] Projection Check\n";
    QVERIFY( rawFirstInRaw.info.projs.size() == rawSecondInRaw.info.projs.size() );

    for( qint32 i = 0; i < rawFirstInRaw.info.projs.size(); ++i )
    {
        std::cout << "Projector " << i << std::endl;
        MatrixXd mTmp = rawFirstInRaw.info.projs[i].data->data - rawSecondInRaw.info.projs[i].data->data;
        QVERIFY( mTmp.sum() < dEpsilon );
    }

    //Compensators
    std::cout << "[3] Compensator Check\n";
    QVERIFY( rawFirstInRaw.info.comps.size() == rawSecondInRaw.info.comps.size() );

    for( qint32 i = 0; i < rawFirstInRaw.info.comps.size(); ++i )
    {
        std::cout << "Compensator " << i << std::endl;
        MatrixXd mTmp = rawFirstInRaw.info.comps[i].data->data - rawSecondInRaw.info.comps[i].data->data;
        QVERIFY( mTmp.sum() < dEpsilon );
    }
}

//=============================================================================================================

void TestFiffRWR::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffRWR)
#include "test_fiff_rwr.moc"
