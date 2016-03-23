//=============================================================================================================
/**
* @file     histogram.cpp
* @author   Ricky Tjen <ricky270@student.sgu.ac.id>;
* @version  1.0
* @date     March, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Ricky Tjen. All rights reserved.
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
#include <vector>
#include <math.h>
#include <cstdlib>
#include <ctime>
#include <Eigen/Dense>
#include <string>
#include <fiff/fiff.h>
#include <mne/mne.h>
#include <QVector>

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

struct vecResultClassLimits
{
    QVector<double> upperClassLimit;
};
struct vecResultFrequency
{
    QVector<int> frequencyNumber;
};

// This function finds the minimum and maximum value from the data matrix and returns the range of raw data
std::vector<double> findRawLocalMinMax(const MatrixXd& data, bool bTransposeOption)
{
    std::vector<double> rawLocalMinMax(4,0.0);
    double rawMin = rawLocalMinMax[0],
           rawMax = rawLocalMinMax[1],
           localMin = rawLocalMinMax[2],
           localMax = rawLocalMinMax[3];

    rawMin = data.minCoeff();    //finds the raw matrix minimum value
    rawMax = data.maxCoeff();    //finds the raw matrix maximum value

    std::cout << "Data range found!" << "\n";
    std::cout << "rawMin =" << rawMin << "\n";
    std::cout << "rawMax =" << rawMax << "\n";

    if (bTransposeOption == true)             //if user chooses to transpose, negative values will be turned into positive
{
        if (rawMin < 0.0)               //in case the raw minimum is a negative value, turn it to positive
        {
            rawMin = abs(rawMin);
            std::cout << "Data transposed!" << "\n";
            std::cout << "New rawMin =" << rawMin << "\n";
        }
        if (rawMax < 0.0)               //in case the raw maximum is a negative value, turn it to positive aswell
        {
            rawMax = abs(rawMax);
            std::cout << "Data transposed!" << "\n";
            std::cout << "New rawMax =" << rawMax << "\n";
        }
        //the following conditional statements are used to ensure that the minimum and maximum are equivalent in length
        if ((rawMin) > rawMax)       //in case the negative side is larger than the positive side
        {
            localMax = rawMin;     //positive side is "stretched" to the exact length as negative side
            localMin = 0.0;
        }

        else if (rawMax > rawMin)  //in case the positive side is larger than the negative side
        {
            localMin = 0.0;       //negative side is "stretched" to the exact length as positive side
            localMax = rawMax;
        }
        else                            //in case both sides are exactly the same
        {
            localMin = 0.0;
            localMax = rawMax;
        }
}

    else
    {
        //if bTransposeOption = false, do the following
        if (abs(rawMin) > rawMax)       //in case the negative side is larger than the positive side
        {
            localMax = abs(rawMin);     //positive side is "stretched" to the exact length as negative side
            localMin = rawMin;
        }

        else if (rawMax > abs(rawMin))  //in case the positive side is larger than the negative side
        {
            localMin = -(rawMax);       //negative side is "stretched" to the exact length as positive side
            localMax = rawMax;
        }
        else                            //in case both sides are exactly the same
        {
            localMin = rawMin;
            localMax = rawMax;
        }
    }
    rawLocalMinMax = {rawMin, rawMax, localMin, localMax};
    return rawLocalMinMax;
}

// This function normalizes the data matrix into desired range before sorting [***untested and not used***]
/**MatrixXd normalize(const MatrixXd& rawMatrix, matrixRange range, std::string transposeOption, vector temp_MatrixRange)
{
    MatrixXd normalizedValue;
    bool doLocalScale = false;
    std::string bTransposeOption = transposeOption;
    std::vector range = findRawLocalMinMax(rawMatrix, transposeOption, temp_MatrixRange);
    if (range.globalMin == 0.0 && range.globalMax == 0.0)       //if global range is NOT given by the user, use local scaling
    {
        doLocalScale = true;
    }

    if (doLocalScale = false)                       //in case global range is given, use global scaling
    {
        for (int ir = 0; ir < rawMatrix.cols(); ir++)
        {
            for (int jr = 0; jr< rawMatrix.rows(); jr++)
            {
             //formula for normalizing values in range [rawMin,rawMax] to range [globalMin,globalMax]
             normalizedValue(ir,jr) = (((range.globalMax - range.globalMin) * rawMatrix(ir,jr))/(range.rawMax - range.rawMin)) + (((range.rawMax * range.globalMin)- (range.rawMin * range.globalMax))/(range.rawMax - range.rawMin));
            }
        }
    }

    if (doLocalScale = true)
    {
        for (int ir = 0; ir < rawMatrix.cols(); ir++)
        {
            for (int jr = 0; jr<rawMatrix.rows(); jr++)
            {
             //formula for normalizing values in range [rawMin,rawMax] to range [localMin,localMax]
             normalizedValue(ir,jr) = (((range.localMax - range.localMin) * rawMatrix(ir,jr))/(range.rawMax - range.rawMin)) + (((range.rawMax * range.localMin)- (range.rawMin * range.localMax))/(range.rawMax - range.rawMin));
            }
        }
    }
    return normalizedValue;  //returns the normalized matrix to main();
}
*/


// This function sorts the data matrix and returns 2 vectors; one is filled with the class limits, the other with class frequency
void sort(const MatrixXd& matPresortedData, bool bTransposeOption, int iClassAmount, QVector<double>& vecResultClassLimits, QVector<int>& vecResultFrequency, double dGlobalMin = 0.0, double dGlobalMax= 0.0)
{
    vecResultClassLimits.clear();
    vecResultFrequency.clear();
    vecResultClassLimits.resize(iClassAmount + 1);
    vecResultFrequency.resize(iClassAmount);

    double desiredMin,
           desiredMax,
           tempValue;
    int ir{0}, jr{0}, kr{0};
    std::vector<double> rawLocalMinMax = findRawLocalMinMax(matPresortedData, bTransposeOption);
    //This if and else function selects either local or global range (according to user preference and input)
    if (dGlobalMin == 0.0 && dGlobalMax == 0.0)       //if global range is NOT given by the user, use local ranges
    {
        desiredMin = rawLocalMinMax.at(2);
        desiredMax = rawLocalMinMax.at(3);
        vecResultClassLimits[0] = desiredMin;                 //replace default value with local minimum at position 0
        vecResultClassLimits[iClassAmount] = desiredMax;     //replace default value with local maximum at position n
        std::cout << "Local Range chosen! \n";
        std::cout << "desiredMin =" << vecResultClassLimits[0] <<"\n";
        std::cout << "desiredMax =" << vecResultClassLimits[iClassAmount] <<"\n";
    }
    else
    {
        desiredMin = dGlobalMin;
        desiredMax = dGlobalMax;
        vecResultClassLimits[0]= desiredMin;                 //replace default value with global minimum at position 0
        vecResultClassLimits[iClassAmount]= desiredMax;     //replace default value with global maximum at position n
        std::cout << "Global Range chosen!\n";
        std::cout << "desiredMin =" << vecResultClassLimits[0] <<"\n";
        std::cout << "desiredMax =" << vecResultClassLimits[iClassAmount] <<"\n";
    }
        double	range = (vecResultClassLimits[iClassAmount] - vecResultClassLimits[0]),                                    //calculates the length from maximum positive value to zero
                dynamicUpperClassLimit;
        for (kr = 0; kr < iClassAmount; kr++)                                          //dynamically initialize the upper class limit values prior to the sorting mecahnism
        {
            dynamicUpperClassLimit = (vecResultClassLimits[0] + (kr*(range/iClassAmount))); //generic formula to determine the upper class limit with respect to range and number of class
            vecResultClassLimits[kr] = dynamicUpperClassLimit;                               //places the appropriate upper class limit value to the right position in the QVector
        }

        if (bTransposeOption == true)     //sort the matrix after turning negative values to positive
        {
            for (ir = 0; ir < matPresortedData.cols(); ir++)              //iterates through all columns of the data matrix
            {
                for (jr = 0; jr<matPresortedData.rows(); jr++)            //iterates through all rows of the data matrix
                {
                    tempValue = abs(matPresortedData(ir,jr));       //turns all values in the matrix into positive
                    for (kr = 0; kr < iClassAmount; kr++)         //starts iteration from 1 to iClassAmount
                    {
                        if (tempValue >= vecResultClassLimits.at(kr) && tempValue < vecResultClassLimits.at(kr + 1))    //compares value in the matrix with lower and upper limit of each class
                        {
                            (vecResultFrequency[kr])++ ; //if the value fits both arguments, the appropriate class frequency is increased by 1
                        }
                    }
                }
            }
        }

        else if (bTransposeOption == false)        //sort the matrix without transposing negative values to positive
        {
            for (ir = 0; ir < matPresortedData.cols(); ir++)        //iterates through every column of the data matrix
            {
                for (jr = 0; jr < matPresortedData.rows(); jr++)      //iterates through every row of the data matrix
                {
                    for (kr = 0; kr < iClassAmount; kr++)         //starts iteration from 1 to iClassAmount
                    {
                        if (matPresortedData(ir,jr) >= vecResultClassLimits.at(kr) && matPresortedData(ir,jr) < vecResultClassLimits.at(kr + 1))    //compares value in the matrix with lower and upper limit of each class
                        {
                            vecResultFrequency[(kr)]++ ; //if the value fits both arguments, the appropriate class frequency is increased by 1
                        }
                    }
                }
            }
        }

    else
    {
        std::cout << "Something went wrong during the sort function.";
    }

}

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

    //
    // histogram calculation
    //User input below
    bool transposeOption;
    transposeOption = true;    //transpose option: false means data is unchanged, true means changing negative values to positive
    int ClassAmount = 100;        //initialize the amount of classes and class frequencies
    double inputGlobalMin = 0.0,
           inputGlobalMax = 0.0;
    QVector<double> resultClassLimits;
    QVector<int> resultFrequency;
    sort(data,transposeOption, ClassAmount, resultClassLimits, resultFrequency, inputGlobalMin, inputGlobalMax );   //user input to normalize and sort the data matrix
    std::cout << "data successfully sorted into desired range and class width...\n";

    //below is the function for printing the results on command prompt (for debugging purposes)
    double lowerClassLimit,
           upperClassLimit;
    int    classFreq,
           totalFreq{0};
    std::cout << "Lower Class Limit\t Upper Class Limit \t Frequency " << std::endl;
    for (int kr=0; kr < resultClassLimits.size()-1; kr++)
    {
        lowerClassLimit = resultClassLimits.at(kr);
        upperClassLimit = resultClassLimits.at(kr+1);
        classFreq = resultFrequency.at(kr);
        std::cout << lowerClassLimit << " \t\t " << upperClassLimit << "\t\t" << classFreq <<std::endl;
        totalFreq = totalFreq + classFreq;
    }

    std::cout << "Total Frequency = " << totalFreq << std::endl;


    return 0;


    std::cout << data.block(0,0,10,10) << std::endl;

    return a.exec();
}



//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================
