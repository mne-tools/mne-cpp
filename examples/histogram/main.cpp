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
struct matrixRange
{
    double rawMin;
    double rawMax;
    double localMin;
    double localMax;
    double globalMin;
    double globalMax;
};
matrixRange temp_MatrixRange = {0.0,0.0,0.0,0.0,0.0,0.0};   // initial value of 0.0 for the variables in matrixRange

struct classInformation
{
    QVector<double> upperClassLimit;
    QVector<int> classFrequency;
};

// This function finds the minimum and maximum value from the data matrix and returns the range of raw data
matrixRange findRawLocalMinMax(const MatrixXd& data, bool transposeOption, matrixRange range = temp_MatrixRange)
{
    double rawMin = range.rawMin,
           rawMax = range.rawMax,
           localMin = range.localMin,
           localMax = range.localMax,
           globalMin = range.globalMin,
           globalMax = range.globalMax;
    bool doTranspose = transposeOption;

    rawMin = data.minCoeff();    //finds the raw matrix minimum value
    rawMax = data.maxCoeff();    //finds the raw matrix maximum value

    std::cout << "Data range found!" << "\n";
    std::cout << "rawMin =" << rawMin << "\n";
    std::cout << "rawMax =" << rawMax << "\n";

    if (doTranspose == true)             //if user chooses to transpose, negative values will be turned into positive
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
        //the following conditional statements are used to ensure that the minimum and maximum
        if ((rawMin) > rawMax)       //in case the negative side is larger than the positive side
        {
            localMax = rawMin;     //positive side is "stretched" to the exact length as negative side
            localMin = rawMin;
        }

        else if (rawMax > rawMin)  //in case the positive side is larger than the negative side
        {
            localMin = rawMax;       //negative side is "stretched" to the exact length as positive side
            localMax = rawMax;
        }
        else                            //in case both sides are exactly the same
        {
            localMin = rawMin;
            localMax = rawMax;
        }

}
                                        //if user chooses not to transpose, the raw values will be left as the original
    else
    {
        //the following conditional statements are used to ensure that the minimum and maximum
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


    std::cout << "localMin =" << localMin << "\n";
    std::cout << "localMax =" << localMax << "\n";
    std::cout << "globalMin =" << globalMin << "\n";
    std::cout << "globalMax =" << globalMax << "\n";
    matrixRange matrixInfo ={rawMin, rawMax, localMin, localMax, globalMin, globalMax};
    return matrixInfo;
}

// This function normalizes the data matrix into desired range before sorting
MatrixXd normalize(const MatrixXd& rawMatrix, matrixRange range, std::string transposeOption)
{
    MatrixXd normalizedValue;
    bool doLocalScale = false;
    std::string doTranspose = transposeOption;
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

// This function sorts the data matrix into classes with width and frequency

sort(const MatrixXd& matPresortedData, matrixRange rawLocalGlobalRange, bool transposeOption, int iClassAmount)
{
    QVector <int>classInformation(iClassAmount);
    double desiredMin,
           desiredMax,
           tempValue;
    bool doTranspose = transposeOption;
    int numberOfClass = 10;

    if (doTranspose == true)     //sort the matrix after turning negative values to positive
    {
        if (rawLocalGlobalRange.globalMin == 0.0 && rawLocalGlobalRange.globalMax == 0.0)       //if global range is NOT given by the user, use local ranges
        {
            desiredMin = rawLocalGlobalRange.localMin;
            desiredMax = rawLocalGlobalRange.localMax;
            std::cout << "Local Range chosen! \n";
            std::cout << "desiredMin =" << desiredMin <<"\n";
            std::cout << "desiredMax =" << desiredMax <<"\n";
        }
        else
        {
            desiredMin = rawLocalGlobalRange.globalMin;
            desiredMax = rawLocalGlobalRange.globalMax;
            std::cout << "Global Range chosen!\n";
            std::cout << "desiredMin =" << desiredMin <<"\n";
            std::cout << "desiredMax =" << desiredMax <<"\n";
        }

        double	range{desiredMax},  //calculates the length from maximum positive value to zero
                upperClassLimit1{(1*(range/numberOfClass)) },   //generic formula to determine the upper class limit with respect to range and number of class


        for (int ir = 0; ir < matPresortedData.cols(); ir++)
        {
            for (int jr = 0; jr<matPresortedData.rows(); jr++)
            {
                tempValue = abs(matPresortedData(ir,jr));

                    if (matPresortedData(ir,jr) > 0.0 && matPresortedData(ir,jr) < upperClassLimit1)
                    {
                        classFrequency1++;
                    }
                    else if (matPresortedData(ir,jr) >= upperClassLimit1 && matPresortedData(ir,jr) < upperClassLimit2)
                    {
                        classFrequency2++;
                    }
                    else if (matPresortedData(ir,jr) >= upperClassLimit2 && matPresortedData(ir,jr) < upperClassLimit3)
                    {
                        classFrequency3++;
                    }
                    else if (matPresortedData(ir,jr) >= upperClassLimit3 && matPresortedData(ir,jr) < upperClassLimit4)
                    {
                        classFrequency4++;
                    }
                    else if (matPresortedData(ir,jr) >= upperClassLimit4 && matPresortedData(ir,jr) < upperClassLimit5)
                    {
                        classFrequency5++;
                    }
                    else if (matPresortedData(ir,jr) >= upperClassLimit5 && matPresortedData(ir,jr) < upperClassLimit6)
                    {
                        classFrequency6++;
                    }
                    else if (matPresortedData(ir,jr) >= upperClassLimit6 && matPresortedData(ir,jr) < upperClassLimit7)
                    {
                        classFrequency7++;
                    }
                    else if (matPresortedData(ir,jr) >= upperClassLimit7 && matPresortedData(ir,jr) < upperClassLimit8)
                    {
                        classFrequency8++;
                    }
                    else if (matPresortedData(ir,jr) >= upperClassLimit8 && matPresortedData(ir,jr) < upperClassLimit9)
                    {
                        classFrequency9++;
                    }
                    else if (matPresortedData(ir,jr) >= upperClassLimit9 && matPresortedData(ir,jr) < upperClassLimit10)
                    {
                            classFrequency10++;
                    }
                }
        }
    }
    else if (doTranspose == false)        //sort the matrix without transposing negative values to positive
    {
        if (rawLocalGlobalRange.globalMin == 0.0 && rawLocalGlobalRange.globalMax == 0.0)       //if global range is NOT given by the user, use local ranges
        {
            desiredMin = rawLocalGlobalRange.localMin;
            desiredMax = rawLocalGlobalRange.localMax;
            std::cout << "Local Range chosen! \n";
            std::cout << "desiredMin =" << desiredMin <<"\n";
            std::cout << "desiredMax =" << desiredMax <<"\n";
        }
        else
        {
            desiredMin = rawLocalGlobalRange.globalMin;
            desiredMax = rawLocalGlobalRange.globalMax;
            std::cout << "Global Range chosen!\n";
            std::cout << "desiredMin =" << desiredMin <<"\n";
            std::cout << "desiredMax =" << desiredMax <<"\n";
        }

        double	range{desiredMax - desiredMin},  //calculates the length of desired range
                upperClassLimit1{ desiredMin + (1*(range/numberOfClass)) },   //generic formula to determine the upper class limit with respect to range and number of class
                upperClassLimit2{ desiredMin + (2*(range/numberOfClass)) },
                upperClassLimit3{ desiredMin + (3*(range/numberOfClass)) },
                upperClassLimit4{ desiredMin + (4*(range/numberOfClass)) },
                upperClassLimit5{ desiredMin + (5*(range/numberOfClass)) },
                upperClassLimit6{ desiredMin + (6*(range/numberOfClass)) },
                upperClassLimit7{ desiredMin + (7*(range/numberOfClass)) },
                upperClassLimit8{ desiredMin + (8*(range/numberOfClass)) },
                upperClassLimit9{ desiredMin + (9*(range/numberOfClass)) },
                upperClassLimit10{ desiredMin + (10*(range/numberOfClass)) };

        for (int ir = 0; ir < matPresortedData.cols(); ir++)
        {
            for (int jr = 0; jr<matPresortedData.rows(); jr++)
            {
                    if (matPresortedData(ir,jr) > desiredMin && matPresortedData(ir,jr) < upperClassLimit1)
                    {
                        classFrequency1++;
                    }
                    else if (matPresortedData(ir,jr) >= upperClassLimit1 && matPresortedData(ir,jr) < upperClassLimit2)
                    {
                        classFrequency2++;
                    }
                    else if (matPresortedData(ir,jr) >= upperClassLimit2 && matPresortedData(ir,jr) < upperClassLimit3)
                    {
                        classFrequency3++;
                    }
                    else if (matPresortedData(ir,jr) >= upperClassLimit3 && matPresortedData(ir,jr) < upperClassLimit4)
                    {
                        classFrequency4++;
                    }
                    else if (matPresortedData(ir,jr) >= upperClassLimit4 && matPresortedData(ir,jr) < upperClassLimit5)
                    {
                        classFrequency5++;
                    }
                    else if (matPresortedData(ir,jr) >= upperClassLimit5 && matPresortedData(ir,jr) < upperClassLimit6)
                    {
                        classFrequency6++;
                    }
                    else if (matPresortedData(ir,jr) >= upperClassLimit6 && matPresortedData(ir,jr) < upperClassLimit7)
                    {
                        classFrequency7++;
                    }
                    else if (matPresortedData(ir,jr) >= upperClassLimit7 && matPresortedData(ir,jr) < upperClassLimit8)
                    {
                        classFrequency8++;
                    }
                    else if (matPresortedData(ir,jr) >= upperClassLimit8 && matPresortedData(ir,jr) < upperClassLimit9)
                    {
                        classFrequency9++;
                    }
                    else if (matPresortedData(ir,jr) >= upperClassLimit9 && matPresortedData(ir,jr) < upperClassLimit10)
                    {
                            classFrequency10++;
                    }
                }
        }
    }

    classInformation classinformation = { upperClassLimit1, upperClassLimit2, upperClassLimit3, upperClassLimit4, upperClassLimit5, upperClassLimit6, upperClassLimit7, upperClassLimit8, upperClassLimit9, upperClassLimit10, classFrequency1, classFrequency2, classFrequency3, classFrequency4, classFrequency5, classFrequency6, classFrequency7, classFrequency8, classFrequency9, classFrequency10 };
    return classinformation;
    }

void printHistogram(classInformation info, matrixRange range)
{
    double minPoint;
    if (range.globalMax == 0.0 && range.globalMin == 0.0)
    {
        minPoint = range.localMin;
    }
    else
    {
        minPoint = range.globalMin;
    }
    std::cout << "Class Width\t\t\t\t Frequency " << std::endl;
    std::cout << minPoint << " to " << info.upperClassLimit1 << "\t\t\t" << info.classFrequency1 <<std::endl;
    std::cout << info.upperClassLimit1 << " to " << info.upperClassLimit2 << "\t\t" << info.classFrequency2 << std::endl;
    std::cout << info.upperClassLimit2 << " to " << info.upperClassLimit3 << "\t\t" << info.classFrequency3 << std::endl;
    std::cout << info.upperClassLimit3 << " to " << info.upperClassLimit4 << "\t\t" << info.classFrequency4 << std::endl;
    std::cout << info.upperClassLimit4 << " to " << info.upperClassLimit5 << "\t\t" << info.classFrequency5 << std::endl;
    std::cout << info.upperClassLimit5 << " to " << info.upperClassLimit6 << "\t\t" << info.classFrequency6 << std::endl;
    std::cout << info.upperClassLimit6 << " to " << info.upperClassLimit7 << "\t\t" << info.classFrequency7 << std::endl;
    std::cout << info.upperClassLimit7 << " to " << info.upperClassLimit8 << "\t\t" << info.classFrequency8 << std::endl;
    std::cout << info.upperClassLimit8 << " to " << info.upperClassLimit9 << "\t\t" << info.classFrequency9 << std::endl;
    std::cout << info.upperClassLimit9 << " to " << info.upperClassLimit10 << "\t\t" << info.classFrequency10 << std::endl;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QFile t_fileRaw("./MNE-sample-data/MEG/sample/sample_audvis_raw.fif");

    float from = 42.956f;
    float to = 44.956f;

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
    std::cout << "Starting histogram calculation...\n";
    std::cout << "Initializing values...\n";
    std::string transposeOption;

    //transpose option: false means data is unchanged, but true means changing negative values to positive
    transposeOption = false;

    //initialize the amount of classes and class frequencies
    int iClassAmount = 10;
    matrixRange rawLocalGlobalRange = findRawLocalMinMax(data, transposeOption, temp_MatrixRange);

//    MatrixXd matPresortedData = normalize(data, rawLocalGlobalRange);
    info = sort(data, rawLocalGlobalRange, transposeOption,iClassAmount);                 //user input to normalize and sort the data matrix
    std::cout << "Sorting data into desired range and class width...\n";
    printHistogram(info, rawLocalGlobalRange);
    std::cout << "Histogram created.\n";
    return 0;


    std::cout << data.block(0,0,10,10) << std::endl;


    return a.exec();
}



//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================
