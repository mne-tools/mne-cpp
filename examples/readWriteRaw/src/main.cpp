//=============================================================================================================
/**
* @file		main.cpp
* @author	Christoph Dinh <christoph.dinh@live.de>;
* @version	1.0
* @date		October, 2010
*
* @section	LICENSE
*
* Copyright (C) 2010 Christoph Dinh. All rights reserved.
*
* No part of this program may be photocopied, reproduced,
* or translated to another program language without the
* prior written consent of the author.
*
*
* @brief	Implements the main() application function.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <math.h>


#include "../../../MNE/fiff/fiff.h"


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

//    QString t_sFile = "./MNE-sample-data/MEG/sample/sample_audvis_raw.fif";//"./MNE-sample-data/test_ctf_raw.fif";
//    QString t_sFile = "./MNE-sample-data/MEG/test_input.fif";
    QString t_sFile = "./MNE-sample-data/MEG/noise-newsystem/noise3.fif";

    QString t_sOutFile = "./MNE-sample-data/MEG/test_output.fif";//"./MNE-sample-data/test_ctf_raw.fif";

    //
    //   Setup for reading the raw data
    //
    FiffRawData* raw = NULL;
    if(!FiffFile::setup_read_raw(t_sFile, raw))
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

    MatrixXi picks = Fiff::pick_types(raw->info, want_meg, want_eeg, want_stim, include, raw->info->bads);
    if(picks.cols() == 0)
    {
        include.clear();
        include << "STI101" << "STI201" << "STI301";
        picks = Fiff::pick_types(raw->info, want_meg, want_eeg, want_stim, include, raw->info->bads);
        if(picks.cols() == 0)
        {
            printf("channel list may need modification\n");
            return -1;
        }
    }
    //
    MatrixXf* cals = NULL;

    FiffFile* outfid = Fiff::start_writing_raw(t_sOutFile,raw->info, cals, picks);
    //
    //   Set up the reading parameters
    //
    fiff_int_t from = raw->first_samp;
    fiff_int_t to = raw->last_samp;
    float quantum_sec = 10.0f;//read and write in 10 sec junks
    fiff_int_t quantum = ceil(quantum_sec*raw->info->sfreq);
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
    MatrixXf* data = NULL;
    MatrixXf* times = NULL;

    for(first = from; first < to; first+=quantum)
    {
        last = first+quantum-1;
        if (last > to)
        {
            last = to;
        }
            //[ data, times ] =

        if (!raw->read_raw_segment(data,times,first,last,picks))
        {
//                fclose(raw.fid);
//                fclose(outfid);
                printf("error during read_raw_segment\n");
                return -1;
        }
        //
        //   You can add your own miracle here
        //
        printf("Writing...\n");
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

    printf("Finished\n");

    return a.exec();
}

//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================


