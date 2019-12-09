//=============================================================================================================
/**
* @file     test_fiff_rwr.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     December, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Test for reading writing reading a fiff raw file
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;

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
    double epsilon;

    FiffRawData first_in_raw;

    MatrixXd first_in_data;
    MatrixXd first_in_times;


    FiffRawData second_in_raw;

    MatrixXd second_in_data;
    MatrixXd second_in_times;
};


//*************************************************************************************************************

TestFiffRWR::TestFiffRWR()
: epsilon(0.000001)
{
}



//*************************************************************************************************************

void TestFiffRWR::initTestCase()
{
    qDebug() << "Epsilon" << epsilon;

    QFile t_fileIn(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif");
    QFile t_fileOut(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_raw_short_test_rwr_out.fif");

    //
    //   Make sure test folder exists
    //
    QFileInfo t_fileOutInfo(t_fileOut);
    QDir().mkdir(t_fileOutInfo.path());


    //*********************************************************************************************************
    // First Read & Write
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Read & Write >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    //
    //   Setup for reading the raw data
    //
    first_in_raw = FiffRawData(t_fileIn);

    //
    //   Set up pick list: MEG + STI 014 - bad channels
    //
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
    //
    qint32 num_of_junks = 5;
    float quantum_sec = 1.0f;//read and write in 1 sec junks
    fiff_int_t from = first_in_raw.first_samp;
    fiff_int_t to = first_in_raw.first_samp + num_of_junks*quantum_sec*first_in_raw.info.sfreq;//raw.last_samp;
    fiff_int_t quantum = ceil(quantum_sec*first_in_raw.info.sfreq);


    //
    //   To read the whole file at once set
    //
    //quantum     = to - from + 1;
    //
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
        //
        //   You can add your own miracle here
        //
        printf("Writing...");
        if (first_buffer)
        {
           if (first > 0)
               outfid->write_int(FIFF_FIRST_SAMPLE,&first);
           first_buffer = false;
        }
        outfid->write_raw_buffer(first_in_data,cals);
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
    second_in_raw = FiffRawData(t_fileOut);

    //
    //   Set up pick list: MEG + STI 014 - bad channels
    //
    //

    picks = second_in_raw.info.pick_types(want_meg, want_eeg, want_stim, include, second_in_raw.info.bads); // prefer member function
    if(picks.cols() == 0)
    {
        include.clear();
        include << "STI101" << "STI201" << "STI301";
        picks = second_in_raw.info.pick_types(want_meg, want_eeg, want_stim, include, second_in_raw.info.bads);// prefer member function
        if(picks.cols() == 0)
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

        if (!second_in_raw.read_raw_segment(second_in_data,second_in_times,first,last/*,picks*/))
        {
                printf("error during read_raw_segment\n");
        }
    }

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Read Again Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}


//*************************************************************************************************************

void TestFiffRWR::compareData()
{
    MatrixXd data_diff = first_in_data - second_in_data;
//    std::cout << "\tCompare data:\n";
//    std::cout << "\tFirst data\n" << first_in_data.block(0,0,4,4) << "\n";
//    std::cout << "\tSecond data\n" << second_in_data.block(0,0,4,4) << "\n";

    QVERIFY( data_diff.sum() < epsilon );
}


//*************************************************************************************************************

void TestFiffRWR::compareTimes()
{
    MatrixXd times_diff = first_in_times - second_in_times;
//    std::cout << "\tCompare Times:\n";
//    std::cout << "\tFirst times\n" << first_in_times.block(0,0,1,4) << "\n";
//    std::cout << "\tSecond times\n" << second_in_times.block(0,0,1,4) << "\n";

    QVERIFY( times_diff.sum() < epsilon );
}


//*************************************************************************************************************

void TestFiffRWR::compareInfo()
{
    //Sampling frequency
    std::cout << "[1] Sampling Frequency Check\n";
    QVERIFY( first_in_raw.info.sfreq == second_in_raw.info.sfreq );

    //Projection
    std::cout << "[2] Projection Check\n";
    QVERIFY( first_in_raw.info.projs.size() == second_in_raw.info.projs.size() );

    for( qint32 i = 0; i < first_in_raw.info.projs.size(); ++i )
    {
        std::cout << "Projector " << i << std::endl;
        MatrixXd tmp = first_in_raw.info.projs[i].data->data - second_in_raw.info.projs[i].data->data;
        QVERIFY( tmp.sum() < epsilon );
    }

    //Compensators
    std::cout << "[3] Compensator Check\n";
    QVERIFY( first_in_raw.info.comps.size() == second_in_raw.info.comps.size() );

    for( qint32 i = 0; i < first_in_raw.info.comps.size(); ++i )
    {
        std::cout << "Compensator " << i << std::endl;
        MatrixXd tmp = first_in_raw.info.comps[i].data->data - second_in_raw.info.comps[i].data->data;
        QVERIFY( tmp.sum() < epsilon );
    }
}

//*************************************************************************************************************

void TestFiffRWR::cleanupTestCase()
{
}


//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffRWR)
#include "test_fiff_rwr.moc"
