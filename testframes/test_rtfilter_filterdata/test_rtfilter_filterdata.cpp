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

#include <utils/ioutils.h>


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
    void compareInfo();
    void cleanupTestCase();

private:
    double epsilon;

    FiffRawData first_in_raw;

    MatrixXd first_in_data;
    MatrixXd first_in_times;
    MatrixXd first_filtered;

    FiffRawData ref_in_raw;

    MatrixXd ref_in_data;
    MatrixXd ref_in_times;
    MatrixXd ref_filtered;
};


//*************************************************************************************************************

TestFiffRFR::TestFiffRFR()
: epsilon(0.000001)
{
}



//*************************************************************************************************************

void TestFiffRFR::initTestCase()
{
    qDebug() << "Epsilon" << epsilon;

    QFile t_fileIn(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_raw_short.fif");
    QFile t_fileRef(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/ref_rtfilter_filterdata_raw.fif");      //Einlesen mne-cpp, schreiben mne-python
    QFile t_fileOut(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/rtfilter_filterdata_out_raw.fif");      //schreiben mne-cpp, einlesen mne-python

    //
    //   Make sure test folder exists
    //

    QFileInfo t_fileOutInfo(t_fileOut);
    QDir().mkdir(t_fileOutInfo.path());


    //*********************************************************************************************************
    // First Read, Filter & Write
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Read, Filter & Write >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    //
    //   Setup for reading the raw data
    //
    first_in_raw = FiffRawData(t_fileIn);

    //
    //   Set up pick list: MEG + STI 014 - bad channels
    //

    bool want_meg   = true;
    bool want_eeg   = false;
    bool want_stim  = false;
    QStringList include;
    include << "STI 014";

    MatrixXi picks = first_in_raw.info.pick_types(want_meg, want_eeg, want_stim, include, first_in_raw.info.bads); // prefer member function
    if(picks.cols() == 0)
    {
        include.clear();
        include << "STI101" << "STI201" << "STI301";
        picks = first_in_raw.info.pick_types(want_meg, want_eeg, want_stim, include, first_in_raw.info.bads);// prefer member function
        if(picks.cols() == 0)
        {
            printf("channel list may need modification\n");
        }
    }
    //
    RowVectorXd cals;

    FiffStream::SPtr outfid = FiffStream::start_writing_raw(t_fileOut,first_in_raw.info, cals/*, picks*/);

    //
    //   Set up the reading parameters
    //   To read the whole file at once set
    //

    fiff_int_t from = first_in_raw.first_samp;
    fiff_int_t to = first_in_raw.last_samp;
    fiff_int_t quantum = to-from+1;
    RtFilter rtFilter;                                                      // filter object

    // channel selection - in this case use every channel
    // size = number of channels; value = index channel number
    QVector<int> channelList(first_in_raw.info.nchan);
    for (int i = 0; i < first_in_raw.info.nchan; i++){
        channelList[i] = i;
    }

    // initialize filter settings
    QString filter_name = "example_cosine";
    FilterData::FilterType type = FilterData::BPF;
    double sFreq = first_in_raw.info.sfreq;                                 // get Sample freq from Data
    double centerfreq = 10/(sFreq/2.0);                                     // normed to nyquist freq.
    double bandwidth = 10/(sFreq/2.0);
    double parkswidth = 1/(sFreq/2.0);
    int order = 8192;
    int fftlength = 16384;
    //
    //   Read and write all the data
    //
    bool first_buffer = true;

    fiff_int_t first, last;

    for(first = from; first < to; first+=quantum)
    {
        last = first+quantum-1;
        if (last > to)
        {
            last = to;
        }

        if (!first_in_raw.read_raw_segment(first_in_data,first_in_times,first,last/*,picks*/))
        {
                printf("error during read_raw_segment\n");
        }

        //Filtering
        printf("Filtering...");
        first_filtered = rtFilter.filterData(first_in_data,type,centerfreq,bandwidth,parkswidth,sFreq,channelList,order, fftlength);
        printf("[done]\n");

        printf("Writing...");
        if (first_buffer)
        {
           if (first > 0)
               outfid->write_int(FIFF_FIRST_SAMPLE,&first);
           first_buffer = false;
        }
        outfid->write_raw_buffer(first_filtered,cals);
        printf("[done]\n");
    }

    outfid->finish_writing_raw();

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Read, Filter & Write Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");


    //*********************************************************************************************************
    // Read MNE-PYTHON Results As Reference
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Read MNE-PYTHON Results As Reference >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    ref_in_raw = FiffRawData(t_fileRef);

    //
    //   Set up pick list: MEG + STI 014 - bad channels
    //
    //

    picks = ref_in_raw.info.pick_types(want_meg, want_eeg, want_stim, include, ref_in_raw.info.bads); // prefer member function
    if(picks.cols() == 0)
    {
        include.clear();
        include << "STI101" << "STI201" << "STI301";
        picks = ref_in_raw.info.pick_types(want_meg, want_eeg, want_stim, include, ref_in_raw.info.bads);// prefer member function
        if(picks.cols() == 0)
        {
            printf("channel list may need modification\n");
        }
    }

    for(first = from; first < to; first+=quantum)
    {
        last = first+quantum-1;
        if (last > to)
        {
            last = to;
        }

        if (!ref_in_raw.read_raw_segment(ref_filtered,ref_in_times,first,last/*,picks*/))
        {
                printf("error during read_raw_segment\n");
        }
    }


//    QString refFileName(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/ref_rtfilter_filterdata_raw.txt");
//    IOUtils::read_eigen_matrix(ref_filtered, refFileName);

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Read MNE-PYTHON Results Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}


//*************************************************************************************************************

void TestFiffRFR::compareData()
{
    MatrixXd data_diff = first_filtered - ref_filtered;
//    std::cout << "\tCompare data:\n";
//    std::cout << "\tFirst data\n" << first_in_data.block(0,0,4,4) << "\n";
//    std::cout << "\tSecond data\n" << second_in_data.block(0,0,4,4) << "\n";

    printf("diff: ..%f",data_diff.sum());
    QVERIFY( data_diff.sum() < epsilon );
}


//*************************************************************************************************************

void TestFiffRFR::compareTimes()
{
    MatrixXd times_diff = first_in_times - ref_in_times;
//    std::cout << "\tCompare Times:\n";
//    std::cout << "\tFirst times\n" << first_in_times.block(0,0,1,4) << "\n";
//    std::cout << "\tSecond times\n" << second_in_times.block(0,0,1,4) << "\n";

    QVERIFY( times_diff.sum() < epsilon );
}


//*************************************************************************************************************

void TestFiffRFR::compareInfo()
{
    //Sampling frequency
    std::cout << "[1] Sampling Frequency Check\n";
    QVERIFY( first_in_raw.info.sfreq == ref_in_raw.info.sfreq );

    //Projection
    std::cout << "[2] Projection Check\n";
    QVERIFY( first_in_raw.info.projs.size() == ref_in_raw.info.projs.size() );

    for( qint32 i = 0; i < first_in_raw.info.projs.size(); ++i )
    {
        std::cout << "Projector " << i << std::endl;
        MatrixXd tmp = first_in_raw.info.projs[i].data->data - ref_in_raw.info.projs[i].data->data;
        QVERIFY( tmp.sum() < epsilon );
    }

    //Compensators
    std::cout << "[3] Compensator Check\n";
    QVERIFY( first_in_raw.info.comps.size() == ref_in_raw.info.comps.size() );

    for( qint32 i = 0; i < first_in_raw.info.comps.size(); ++i )
    {
        std::cout << "Compensator " << i << std::endl;
        MatrixXd tmp = first_in_raw.info.comps[i].data->data - ref_in_raw.info.comps[i].data->data;
        QVERIFY( tmp.sum() < epsilon );
    }
}

//*************************************************************************************************************

void TestFiffRFR::cleanupTestCase()
{
}


//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffRFR)
#include "test_rtfilter_filterdata.moc"
