//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Christoph Dinh, Gabriel B Motta, Lorenz Esch. All rights reserved.
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
 * @brief     Example of the FiffIO interface class
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <math.h>

#include <fiff/fiff.h>
#include <mne/mne.h>

#include <fiff/fiff_io.h>

//Eigen
#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace MNELIB;
using namespace Eigen;

//=============================================================================================================
// MAIN
//=============================================================================================================

//=============================================================================================================
/**
 * The function main marks the entry point of the program.
 * By default, main has the storage class extern.
 *
 * @param[in] argc (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
 * @param[in] argv (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
 * @return the value that was set to exit() (which is 0 if exit() is called via quit()).
 */
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    //create list of fiff data to read
    QList<QIODevice*> t_listSampleFilesIn;
    t_listSampleFilesIn.append(new QFile(QCoreApplication::applicationDirPath() + "/../data/MNE-sample-data/MEG/sample/sample_audvis_raw.fif"));
//    t_listSampleFilesIn.append(new QFile(QCoreApplication::applicationDirPath() + "/../data/MNE-sample-data/MEG/sample/sample_audvis_raw.fif"));
//    t_listSampleFilesIn.append(new QFile(QCoreApplication::applicationDirPath() + "/../data/MNE-sample-data/MEG/sample/sample_audvis-ave.fif"));
//    t_listSampleFilesIn.append(new QFile(QCoreApplication::applicationDirPath() + "/../data/MNE-sample-data/MEG/sample/sample_audvis-no-filter-ave.fif"));
//    t_listSampleFilesIn.append(new QFile(QCoreApplication::applicationDirPath() + "/../data/MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif"));
//    t_listSampleFilesIn.append(new QFile(QCoreApplication::applicationDirPath() + "/../data/MNE-sample-data/MEG/sample/sample_audvis-cov.fif"));

    FiffIO p_fiffIO(t_listSampleFilesIn);

    std::cout << p_fiffIO << std::endl;

    bool readSuccessful = false;

    //Read raw data samples
    MatrixXd data,times;
    readSuccessful= p_fiffIO.m_qlistRaw[0]->read_raw_segment_times(data,times,100,102);

    if (readSuccessful){
        //Write some raw data
        QFile t_fileToWrite(QCoreApplication::applicationDirPath() + "/../data/MNE-sample-data/MEG/sample/sample_write/sample_out.fif");
        p_fiffIO.write(t_fileToWrite,FIFFB_RAW_DATA,-1);
    } else {
        qDebug() << "Could not read raw segment.";
    }

    return a.exec();
}

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================
