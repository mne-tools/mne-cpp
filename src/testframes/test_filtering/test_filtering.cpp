//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2019-2026 MNE-CPP Authors
 *
 * @file     test_filtering.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Ruben Doerfel <doerfelruben@aol.com>
 * @since    0.1.0
 * @date     12, 2019
 * @brief     test for filterData function that calls rtproceesing and utils library.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/mne_logger.h>

#include <iostream>
#include <vector>
#include <math.h>

#include <fiff/fiff.h>
#include <dsp/filterkernel.h>
#include <dsp/rt/rt_filter.h>

#include <Eigen/Dense>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QFile>
#include <QCommandLineParser>
#include <QtTest>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace RTPROCESSINGLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestFiltering
 *
 * @brief The TestFiltering class provides read filter read fiff verification tests
 *
 */
class TestFiltering: public QObject
{
    Q_OBJECT

public:
    TestFiltering();

private slots:
    void initTestCase();
    void compareData();
    void compareTimes();
    void cleanupTestCase();

private:
    double dEpsilon;
    int iOrder;

    MatrixXd mFirstInData;
    MatrixXd mFirstInTimes;
    MatrixXd mFirstFiltered;

    MatrixXd mRefInData;
    MatrixXd mRefInTimes;
    MatrixXd mRefFiltered;

};

//=============================================================================================================

TestFiltering::TestFiltering()
: dEpsilon(0.000001)
{
}

//=============================================================================================================

void TestFiltering::initTestCase()
{
    qInstallMessageHandler(UTILSLIB::MNELogger::customLogWriter);
    qDebug() << "Epsilon" << dEpsilon;

    QFile t_fileIn(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif");
    QFile t_fileOut(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/rtfilter_filterdata_out_raw.fif");

    // Filter in Python is created with following function: mne.filter.design_mne_c_filter(raw.info['sfreq'], 5, 10, 1, 1)
    // This will create a filter with with 8193 elements/taps/Order. In order to be concise with the MNE-CPP implementation
    // the filter is cut to the Order used in mne-cpp (1024, see below).//
    // The actual filtering was performed with the function: mne.filter._overlap_add_filter(dataIn, filter_python, phase = 'linear')
    QFile t_fileRef(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/ref_rtfilter_filterdata_raw.fif");

    // Make sure test folder exists
    QFileInfo t_fileOutInfo(t_fileOut);
    QDir().mkdir(t_fileOutInfo.path());

    //*********************************************************************************************************
    // First Read, Filter & Write
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Read, Filter & Write >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    // Setup for reading the raw data
    FiffRawData rawFirstInRaw;
    rawFirstInRaw = FiffRawData(t_fileIn);

    // Only filter MEG channels
    RowVectorXi vPicks = rawFirstInRaw.info.pick_types(true, true, false);
    RowVectorXd vCals;
    FiffStream::SPtr outfid = FiffStream::start_writing_raw(t_fileOut, rawFirstInRaw.info, vCals);

    //   Set up the reading parameters
    //   To read the whole file at once set

    fiff_int_t from = rawFirstInRaw.first_samp;
    fiff_int_t to = rawFirstInRaw.last_samp;

    // initialize filter settings
    QString sFilterName = "example_cosine";
    int type = FilterKernel::m_filterTypes.indexOf(FilterParameter("BPF"));
    double dSFreq = rawFirstInRaw.info.sfreq;
    double dCenterfreq = 10;
    double dBandwidth = 10;
    double dTransition = 1;
    iOrder = 1024;

    MatrixXd mDataFiltered;

    // Reading
    if(!rawFirstInRaw.read_raw_segment(mFirstInData, mFirstInTimes, from, to)) {
        printf("error during read_raw_segment\n");
    }

    // Filtering
    printf("Filtering...");
    mFirstFiltered = RTPROCESSINGLIB::filterData(mFirstInData,
                                                 type,
                                                 dCenterfreq,
                                                 dBandwidth,
                                                 dTransition,
                                                 dSFreq,
                                                 1024,
                                                 UTILSLIB::FilterKernel::m_designMethods.indexOf(FilterParameter("Cosine")),
                                                 vPicks);
    printf("[done]\n");

    // Writing
    printf("Writing...");
    outfid->write_int(FIFF_FIRST_SAMPLE, &from);
    outfid->write_raw_buffer(mFirstFiltered,vCals);
    printf("[done]\n");

    outfid->finish_writing_raw();

    // Read filtered data from the filtered output file to check if read and write is working correctly
    FiffRawData rawSecondInRaw;
    rawSecondInRaw = FiffRawData(t_fileOut);

    // Reading
    if (!rawSecondInRaw.read_raw_segment(mFirstFiltered,mFirstInTimes,from,to,vPicks)) {
        printf("error during read_raw_segment\n");
    }

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Read, Filter & Write Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");

    //*********************************************************************************************************
    // Read MNE-PYTHON Results As Reference
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Read MNE-PYTHON Results As Reference >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    FiffRawData ref_in_raw;
    ref_in_raw = FiffRawData(t_fileRef);

    // Reading
    if (!ref_in_raw.read_raw_segment(mRefFiltered,mRefInTimes,from,to,vPicks)) {
        printf("error during read_raw_segment\n");
    }

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Read MNE-PYTHON Results Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//=============================================================================================================

void TestFiltering::compareData()
{
    //make sure to only read data after 1/2 filter Length
    int iLength = mFirstFiltered.cols()-int(iOrder/2);
    MatrixXd mDataDiff = mFirstFiltered.block(0,int(iOrder/2),mFirstFiltered.rows(),iLength) - mRefFiltered.block(0,int(iOrder/2),mRefFiltered.rows(),iLength);
    QVERIFY( mDataDiff.sum() < dEpsilon );
}

//=============================================================================================================

void TestFiltering::compareTimes()
{
    MatrixXd mTimesDiff = mFirstInTimes - mRefInTimes;
    QVERIFY( mTimesDiff.sum() < dEpsilon );
}

void TestFiltering::cleanupTestCase()
{
    QFile t_fileOut(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/rtfilter_filterdata_out_raw.fif");
    t_fileOut.remove();
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiltering)
#include "test_filtering.moc"
