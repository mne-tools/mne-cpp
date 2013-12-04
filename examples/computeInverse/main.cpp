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
* @brief    Example of computing a L2 minimum-norm estimate or a dSPM solution
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <math.h>


//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include <fiff/fiff_evoked_set.h>
#include <mne/mne_inverse_operator.h>

#include <fiff/fiff_evoked.h>
#include <mne/mne_sourceestimate.h>
#include <inverse/minimumNorm/minimumnorm.h>


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
using namespace MNELIB;
using namespace INVERSELIB;


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

//  fname_data  - Name of the data file
//  setno       - Data set number
//  fname_inv   - Inverse operator file name
//  nave        - Number of averages (scales the noise covariance)
//             If negative, the number of averages in the data will be
//             used
//  lambda2     - The regularization factor
//  dSPM        - do dSPM?
//  sLORETA     - do sLORETA?

//    QFile t_fileEvoked("./MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
//    QFile t_fileInv("./MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-meg-eeg-inv.fif");

    QFile t_fileEvoked("E:/Data/sl_data/MEG/mind006/mind006_051209_auditory01_raw-ave.fif");
    QFile t_fileInv("E:/Data/sl_data/MEG/mind006/mind006_051209_auditory01_raw-oct-6p-meg-inv.fif");

    float snr = 3.0f;
    QString method("dSPM"); //"MNE" | "dSPM" | "sLORETA"
    QString t_sFileNameStc("");

    // Parse command line parameters
    for(qint32 i = 0; i < argc; ++i)
    {
        if(strcmp(argv[i], "-snr") == 0 || strcmp(argv[i], "--snr") == 0)
        {
            if(i + 1 < argc)
                snr = atof(argv[i+1]);
        }
        else if(strcmp(argv[i], "-method") == 0 || strcmp(argv[i], "--method") == 0)
        {
            if(i + 1 < argc)
                method = QString::fromUtf8(argv[i+1]);
        }
        else if(strcmp(argv[i], "-stc") == 0 || strcmp(argv[i], "--stc") == 0)
        {
            if(i + 1 < argc)
                t_sFileNameStc = QString::fromUtf8(argv[i+1]);
        }
    }

    double lambda2 = 1.0 / pow(snr, 2);
    qDebug() << "Start calculation with: SNR" << snr << "; Lambda" << lambda2 << "; Method" << method << "; stc:" << t_sFileNameStc;

    //
    //   Read the data first
    //
    fiff_int_t setno = 0;
    QPair<QVariant, QVariant> baseline(QVariant(), 0);
    FiffEvoked evoked(t_fileEvoked, setno, baseline);
    if(evoked.isEmpty())
        return 1;

    //
    //   Then the inverse operator
    //
    MNEInverseOperator inverse_operator(t_fileInv);

    //
    // Compute inverse solution
    //
    MinimumNorm minimumNorm(inverse_operator, lambda2, method);
    MNESourceEstimate sourceEstimate = minimumNorm.calculateInverse(evoked);

    //
    //Results
    //
    std::cout << "\npart ( block( 0, 0, 10, 10) ) of the inverse solution:\n" << sourceEstimate.data.block(0,0,10,10) << std::endl;
    printf("tmin = %f s\n", sourceEstimate.tmin);
    printf("tstep = %f s\n", sourceEstimate.tstep);


    if(!t_sFileNameStc.isEmpty())
    {
        QFile t_fileStc(t_sFileNameStc);
        sourceEstimate.write(t_fileStc);

        //test if everything was written correctly
        MNESourceEstimate readSourceEstimate(t_fileStc);

        std::cout << "\npart ( block( 0, 0, 10, 10) ) of the inverse solution:\n" << readSourceEstimate.data.block(0,0,10,10) << std::endl;
        printf("tmin = %f s\n", readSourceEstimate.tmin);
        printf("tstep = %f s\n", readSourceEstimate.tstep);
    }

    return 0;//a.exec();
}
