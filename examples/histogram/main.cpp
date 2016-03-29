//=============================================================================================================
/**
* @file     histogram.cpp
* @author   Ricky Tjen <ricky270@student.sgu.ac.id>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Ricky Tjen and Matti Hamalainen. All rights reserved.
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
* @brief    Example of reading raw data
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <math.h>
#include <ctime>
#include <cstdlib>
#include <Eigen/Dense>
#include <string>
#include <fiff/fiff.h>
#include <mne/mne.h>
#include <utils/histogram.h>
#include <QDebug>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QVector>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace MNELIB;
using namespace std;

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

// This function normalizes the data matrix into desired range before sorting [***untested and not used***]
/**MatrixXd normalize(const MatrixXd& rawMatrix, matrixRange range, std::string transposeOption, vector temp_MatrixRange)
*{
*    MatrixXd normalizedValue;
*    bool doLocalScale = false;
*    std::string bTransposeOption = transposeOption;
*    std::vector range = findRawLocalMinMax(rawMatrix, transposeOption, temp_MatrixRange);
*    if (range.globalMin == 0.0 && range.globalMax == 0.0)       //if global range is NOT given by the user, use local scaling
*    {
*        doLocalScale = true;
*    }
*
*    if (doLocalScale = false)                       //in case global range is given, use global scaling
*    {
*        for (int ir = 0; ir < rawMatrix.cols(); ir++)
*        {
*            for (int jr = 0; jr< rawMatrix.rows(); jr++)
*            {
*             //formula for normalizing values in range [rawMin,rawMax] to range [globalMin,globalMax]
*            normalizedValue(ir,jr) = (((range.globalMax - range.globalMin) * rawMatrix(ir,jr))/(range.rawMax - range.rawMin)) + (((range.rawMax * range.globalMin)- (range.rawMin * range.globalMax))/(range.rawMax - range.rawMin));
*            }
*        }
*    }
*
*    if (doLocalScale = true)
*    {
*        for (int ir = 0; ir < rawMatrix.cols(); ir++)
*        {
*            for (int jr = 0; jr<rawMatrix.rows(); jr++)
*            {
*             //formula for normalizing values in range [rawMin,rawMax] to range [localMin,localMax]
*             normalizedValue(ir,jr) = (((range.localMax - range.localMin) * rawMatrix(ir,jr))/(range.rawMax - range.rawMin)) + (((range.rawMax * range.localMin)- (range.rawMin * range.localMax))/(range.rawMax - range.rawMin));
*            }
*        }
*    }
*    return normalizedValue;  //returns the normalized matrix to main();
*}
**/

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QFile t_fileRaw("./MNE-sample-data/MEG/sample/sample_audvis_raw.fif");

    float from = 35.956f;
    float to = 45.123f;

    bool in_samples = false;

    bool keep_comp = true;

    //
    //   Setup for reading the raw data
    //
    FiffRawData raw(t_fileRaw);
    
    //
    //   Set up pick list: MEG + STI 014 - bad channels
    //
    //
    QStringList include;
    include << "STI 014";
    bool want_meg   = true;
    bool want_eeg   = false;
    bool want_stim  = false;

    RowVectorXi picks = raw.info.pick_types(want_meg, want_eeg, want_stim, include, raw.info.bads);

    //
    //   Set up projection
    //
    qint32 k = 0;
    if (raw.info.projs.size() == 0)
        printf("No projector specified for these data\n");
    else
    {
        //
        //   Activate the projection items
        //
        for (k = 0; k < raw.info.projs.size(); ++k)
            raw.info.projs[k].active = true;

        printf("%d projection items activated\n",raw.info.projs.size());
        //
        //   Create the projector
        //
        fiff_int_t nproj = raw.info.make_projector(raw.proj);

        if (nproj == 0)
            printf("The projection vectors do not apply to these channels\n");
        else
            printf("Created an SSP operator (subspace dimension = %d)\n",nproj);
    }

    //
    //   Set up the CTF compensator
    //
    qint32 current_comp = raw.info.get_current_comp();
    qint32 dest_comp = -1;

    if (current_comp > 0)
        printf("Current compensation grade : %d\n",current_comp);

    if (keep_comp)
        dest_comp = current_comp;

    if (current_comp != dest_comp)
    {
        qDebug() << "This part needs to be debugged";
        if(MNE::make_compensator(raw.info, current_comp, dest_comp, raw.comp))
        {
            raw.info.set_current_comp(dest_comp);
            printf("Appropriate compensator added to change to grade %d.\n",dest_comp);
        }
        else
        {
            printf("Could not make the compensator\n");
            return -1;
        }
    }
    //
    //   Read a data segment
    //   times output argument is optional
    //
    bool readSuccessful = false;
    MatrixXd data;
    MatrixXd times;
    if (in_samples)
        readSuccessful = raw.read_raw_segment(data, times, (qint32)from, (qint32)to, picks);
    else
        readSuccessful = raw.read_raw_segment_times(data, times, from, to, picks);

    if (!readSuccessful)
    {
        printf("Could not read raw segment.\n");
        return -1;
    }

    printf("Read %d samples.\n",(qint32)data.cols());

    // histogram calculation
    bool transposeOption;
    transposeOption = false;    //transpose option: false means data is unchanged, true means changing negative values to positive
    int ClassAmount = 20;        //initialize the amount of classes and class frequencies
    double inputGlobalMin = 0.0,
           inputGlobalMax = 0.0;
    QVector<double> resultClassLimits;
    QVector<int> resultFrequency;
    Histogram::sort(data,transposeOption, ClassAmount, resultClassLimits, resultFrequency, inputGlobalMin, inputGlobalMax );   //user input to normalize and sort the data matrix
    qDebug() << "data successfully sorted into desired range and class width...\n";

    //below is the function for printing the results on command prompt (for debugging purposes)
    double lowerClassLimit,
           upperClassLimit;
    int    classFreq,
           totalFreq{0};
    qDebug() << "Lower Class Limit\t Upper Class Limit \t Frequency ";
    for (int kr=0; kr < resultClassLimits.size()-1; kr++)
    {
        lowerClassLimit = resultClassLimits.at(kr);
        upperClassLimit = resultClassLimits.at(kr+1);
        classFreq = resultFrequency.at(kr);
        qDebug() << lowerClassLimit << " \t\t " << upperClassLimit << "\t\t" << classFreq;
        totalFreq = totalFreq + classFreq;
    }

    qDebug() << "Total Frequency = " << totalFreq;


    return 0;


    std::cout << data.block(0,0,10,10);

    return a.exec();
}



//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================
