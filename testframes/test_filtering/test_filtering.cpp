//=============================================================================================================
/**
 * @file     test_filtering.cpp
 * @author   Ruben Doerfel <Ruben.Doerfel@tu-ilmenau.de>
 * @since    0.1.0
 * @date     12, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Ruben Doerfel. All rights reserved.
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
 * @brief     test for filterData function that calls rtproceesing and utils library.
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
#include <rtprocessing/helpers/filterkernel.h>
#include <rtprocessing/filter.h>

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
    qInstallMessageHandler(UTILSLIB::ApplicationLogger::customLogWriter);
    qDebug() << "Epsilon" << dEpsilon;

    QFile t_fileIn(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif");
    QFile t_fileOut(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/rtfilter_filterdata_out_raw.fif");

    // Filter in Python is created with following function: mne.filter.design_mne_c_filter(raw.info['sfreq'], 5, 10, 1, 1)
    // This will create a filter with with 8193 elements/taps/Order. In order to be concise with the MNE-CPP implementation
    // the filter is cut to the Order used in mne-cpp (1024, see below).//
    // The actual filtering was performed with the function: mne.filter._overlap_add_filter(dataIn, filter_python, phase = 'linear')
    QFile t_fileRef(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/Result/ref_rtfilter_filterdata_raw.fif");

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
                                                 RTPROCESSINGLIB::FilterKernel::m_designMethods.indexOf(FilterParameter("Cosine")),
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
    QFile t_fileOut(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/rtfilter_filterdata_out_raw.fif");
    t_fileOut.remove();
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiltering)
#include "test_filtering.moc"
