//=============================================================================================================
/**
* @file     main.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Implements the main() application function.
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


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;


//*************************************************************************************************************
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
    QCoreApplication a(argc, argv);

    QString t_sFileName = "./MNE-sample-data/MEG/sample/sample_audvis_raw.fif";
//    QString t_sFile = "./MNE-sample-data/MEG/test_input.fif";

    QFile* t_pFile = new QFile(t_sFileName);

    QString t_sOutFileName = "./MNE-sample-data/MEG/test_output.fif";

    QFile* t_pOutFile = new QFile(t_sOutFileName);

    //
    //   Setup for reading the raw data
    //
    FiffRawData* raw = NULL;
    if(!FiffStream::setup_read_raw(t_pFile, raw))
    {
        printf("Error during fiff setup raw read");
        return 0;
    }
    //
    //   Set up pick list: MEG + STI 014 - bad channels
    //
    //
    bool want_meg   = true;
    bool want_eeg   = false;
    bool want_stim  = false;
    QStringList include;
    include << "STI 014";

//    MatrixXi picks = Fiff::pick_types(raw->info, want_meg, want_eeg, want_stim, include, raw->info->bads);
    MatrixXi picks = raw->info.pick_types(want_meg, want_eeg, want_stim, include, raw->info.bads); // prefer member function
    if(picks.cols() == 0)
    {
        include.clear();
        include << "STI101" << "STI201" << "STI301";
//        picks = Fiff::pick_types(raw->info, want_meg, want_eeg, want_stim, include, raw->info->bads);
        picks = raw->info.pick_types(want_meg, want_eeg, want_stim, include, raw->info.bads);// prefer member function
        if(picks.cols() == 0)
        {
            printf("channel list may need modification\n");
            return -1;
        }
    }
    //
    MatrixXd* cals = NULL;

    FiffStream* outfid = Fiff::start_writing_raw(t_pOutFile,raw->info, cals, picks);
    //
    //   Set up the reading parameters
    //
    fiff_int_t from = raw->first_samp;
    fiff_int_t to = raw->last_samp;
    float quantum_sec = 10.0f;//read and write in 10 sec junks
    fiff_int_t quantum = ceil(quantum_sec*raw->info.sfreq);
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
    MatrixXd* data = NULL;
    MatrixXd* times = NULL;

    for(first = from; first < to; first+=quantum)
    {
        last = first+quantum-1;
        if (last > to)
        {
            last = to;
        }

        if (!raw->read_raw_segment(data,times,first,last,picks))
        {
                delete raw;
                delete outfid;
                printf("error during read_raw_segment\n");
                return -1;
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
        outfid->write_raw_buffer(data,cals);
        printf("[done]\n");
    }

    outfid->finish_writing_raw();


    delete raw;
    delete outfid;
    delete t_pOutFile;
    delete t_pFile;

    printf("Finished\n");

    return a.exec();
}

//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================


