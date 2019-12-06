//=============================================================================================================
/**
* @file     main.cpp
* @author   Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     12, 2019
*
* @section  LICENSE
*
* Copyright (C) 2019, Ruben Dörfel and Matti Hamalainen. All rights reserved.
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <math.h>

#include <fiff/fiff.h>
#include <utils/filterTools/filterdata.h>
#include <rtprocessing/rtfilter.h>

#include <Eigen/Dense>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QFile>
#include <QCommandLineParser>
#include <QtTest>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace RTPROCESSINGLIB;

//=============================================================================================================
/**
* DECLARE CLASS TestFiffRFR
*
* @brief The TestFiffRFR class provides read filter read fiff verification tests
*
*/
class TestFiffRFR: public QObject
{
    Q_OBJECT

public:
    TestFiffRFR();

private slots:
    void initTestCase();
    void compareData();
    void compareTimes();
    void cleanupTestCase();

private:
    double epsilon;
    int order;

    MatrixXd first_in_data;
    MatrixXd first_in_times;
    MatrixXd first_filtered;

    MatrixXd ref_in_data;
    MatrixXd ref_in_times;
    MatrixXd ref_filtered;

    MatrixXi picks;
};

//*************************************************************************************************************

TestFiffRFR::TestFiffRFR()
: epsilon(0.000001)
{
}

//*************************************************************************************************************

void TestFiffRFR::initTestCase() {

    qDebug() << "Epsilon" << epsilon;

    QFile t_fileIn(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif");
    QFile t_fileOut(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/rtfilter_filterdata_out_raw.fif");

    // Filter in Python is created with following function: mne.filter.design_mne_c_filter(raw.info['sfreq'], 5, 10, 1, 1)
    // This will create a filter with with 8193 elements/taps/order. In order to be concise with the MNE-CPP implementation
    // the filter is cut to the order used in mne-cpp (1024, see below).//
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
    FiffRawData first_in_raw;
    first_in_raw = FiffRawData(t_fileIn);

    // Set up pick list: MEG - bad channels
    bool want_meg   = true;
    bool want_eeg   = false;
    bool want_stim  = false;

    RowVectorXi picks = first_in_raw.info.pick_types(true, false, false, QStringList(), first_in_raw.info.bads);
    RowVectorXd cals;
    FiffStream::SPtr outfid = FiffStream::start_writing_raw(t_fileOut, first_in_raw.info, cals);

    //   Set up the reading parameters
    //   To read the whole file at once set

    fiff_int_t from = first_in_raw.first_samp;
    fiff_int_t to = first_in_raw.last_samp;
    fiff_int_t quantum = to-from+1;
    RtFilter rtFilter;

    // initialize filter settings
    QString filter_name = "example_cosine";
    FilterData::FilterType type = FilterData::BPF;
    double sFreq = first_in_raw.info.sfreq;
    double centerfreq = 10;
    double bandwidth = 10;
    double parkswidth = 1;
    order = 1024;
    int fftlength = 4096;

    // Read and write all the data
    bool first_buffer = true;

    fiff_int_t first, last;

    for(first = from; first < to; first+=quantum) {
        last = first+quantum-1;

        if (last > to) {
            last = to;
        }

        if (!first_in_raw.read_raw_segment(first_in_data,first_in_times,first,last)) {
            printf("error during read_raw_segment\n");
        }

        //Filtering
        printf("Filtering...");
        first_filtered = rtFilter.filterData(first_in_data,
                                             type,
                                             centerfreq,
                                             bandwidth,
                                             parkswidth,
                                             sFreq,
                                             picks,
                                             order,
                                             fftlength);
        printf("[done]\n");

        printf("Writing...");
        if (first_buffer) {
           if (first > 0)
               outfid->write_int(FIFF_FIRST_SAMPLE,&first);
           first_buffer = false;
        }
        outfid->write_raw_buffer(first_filtered,cals);
        printf("[done]\n");
    }

    outfid->finish_writing_raw();

    // Read filtered data from the filtered output file to check if read and write is working correctly
    FiffRawData second_in_Raw;
    second_in_Raw = FiffRawData(t_fileOut);

    picks = second_in_Raw.info.pick_types(want_meg, want_eeg, want_stim, include, bads); // prefer member function

    for(first = from; first < to; first+=quantum) {
        last = first+quantum-1;

        if (last > to) {
            last = to;
        }
        if (!second_in_Raw.read_raw_segment(first_filtered,first_in_times,first,last,picks)) {
            printf("error during read_raw_segment\n");
        }
    }

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Read, Filter & Write Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");

    //*********************************************************************************************************
    // Read MNE-PYTHON Results As Reference
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Read MNE-PYTHON Results As Reference >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    FiffRawData ref_in_raw;
    ref_in_raw = FiffRawData(t_fileRef);

    picks = ref_in_raw.info.pick_types(want_meg, want_eeg, want_stim, include, bads); // prefer member function

    for(first = from; first < to; first+=quantum) {
        last = first+quantum-1;

        if (last > to)
        {
            last = to;
        }
        if (!ref_in_raw.read_raw_segment(ref_filtered,ref_in_times,first,last,picks))
        {
            printf("error during read_raw_segment\n");
        }
    }

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Read MNE-PYTHON Results Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//*************************************************************************************************************

void TestFiffRFR::compareData() {

    //make sure to only read data after 1/2 filter length
    int length = first_filtered.cols()-int(order/2);
    MatrixXd data_diff = first_filtered.block(0,int(order/2),first_filtered.rows(),length) - ref_filtered.block(0,int(order/2),ref_filtered.rows(),length);
    QVERIFY( data_diff.sum() < epsilon );

}

//*************************************************************************************************************

void TestFiffRFR::compareTimes() {

    MatrixXd times_diff = first_in_times - ref_in_times;
    QVERIFY( times_diff.sum() < epsilon );

}

void TestFiffRFR::cleanupTestCase()
{
}

//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffRFR)
#include "test_rtfilter_filterdata.moc"
