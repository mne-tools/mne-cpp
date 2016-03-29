//=============================================================================================================
/**
* @file     histogram.cpp
* @author   Ricky Tjen <ricky270@student.sgu.ac.id>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
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
* @brief    Implementation of histogram class.
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "histogram.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;



//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void Histogram::sort(const Eigen::MatrixXd& matPresortedData, bool bTransposeOption, int iClassAmount, QVector<double>& vecResultClassLimits, QVector<int>& vecResultFrequency, double dGlobalMin, double dGlobalMax)
{
    vecResultClassLimits.clear();
    vecResultFrequency.clear();
    vecResultClassLimits.resize(iClassAmount + 1);
    vecResultFrequency.resize(iClassAmount);

    double desiredMin,
           desiredMax,
           tempValue;
    int ir{0}, jr{0}, kr{0};
    QVector<double> rawLocalMinMax = findRawLocalMinMax(matPresortedData, bTransposeOption);
    //This if and else function selects either local or global range (according to user preference and input)
    if (dGlobalMin == 0.0 && dGlobalMax == 0.0)       //if global range is NOT given by the user, use local ranges
    {
        desiredMin = rawLocalMinMax.at(2);
        desiredMax = rawLocalMinMax.at(3);
        vecResultClassLimits[0] = desiredMin;                 //replace default value with local minimum at position 0
        vecResultClassLimits[iClassAmount] = desiredMax;     //replace default value with local maximum at position n
        qDebug() << "Local Range chosen! \n";
        qDebug() << "desiredMin =" << vecResultClassLimits[0];
        qDebug() << "desiredMax =" << vecResultClassLimits[iClassAmount];
    }
    else
    {
        desiredMin = dGlobalMin;
        desiredMax = dGlobalMax;
        vecResultClassLimits[0]= desiredMin;                 //replace default value with global minimum at position 0
        vecResultClassLimits[iClassAmount]= desiredMax;     //replace default value with global maximum at position n
        qDebug() << "Global Range chosen!\n";
        qDebug() << "desiredMin =" << vecResultClassLimits[0];
        qDebug() << "desiredMax =" << vecResultClassLimits[iClassAmount];
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
        qDebug() << "Something went wrong during the sort function.";
    }

}


//*************************************************************************************************************

QVector<double> Histogram::findRawLocalMinMax(const Eigen::MatrixXd& matData, bool bTransposeOption)
{
    QVector<double> rawLocalMinMax(4,0.0);
    double rawMin = rawLocalMinMax[0],
           rawMax = rawLocalMinMax[1],
           localMin = rawLocalMinMax[2],
           localMax = rawLocalMinMax[3];

    rawMin = matData.minCoeff();    //finds the raw matrix minimum value
    rawMax = matData.maxCoeff();    //finds the raw matrix maximum value

    qDebug() << "Data range found";
    qDebug() << "rawMin =" << rawMin;
    qDebug() << "rawMax =" << rawMax;

    if (bTransposeOption == true)             //if user chooses to transpose, negative values will be turned into positive
{
        if (rawMin < 0.0)               //in case the raw minimum is a negative value, turn it to positive
        {
            rawMin = abs(rawMin);
            qDebug() << "Data transposed!";
            qDebug() << "New rawMin =" << rawMin;
        }
        if (rawMax < 0.0)               //in case the raw maximum is a negative value, turn it to positive aswell
        {
            rawMax = abs(rawMax);
            qDebug() << "Data transposed!";
            qDebug() << "New rawMax =" << rawMax;
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

