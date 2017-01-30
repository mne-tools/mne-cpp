//=============================================================================================================
/**
* @file     calcMetric.cpp
* @author   Louis Eichhorst <louis.eichhorst@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     Month, Year
*
* @section  LICENSE
*
* Copyright (C) Year, Your name and Matti Hamalainen. All rights reserved.
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
* @brief    calcMetric class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "calcMetric.h"
#include <thread>


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QElapsedTimer>
#include <QtConcurrent/QtConcurrent>
#include <QtConcurrent/QtConcurrentMap>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

//using namespace IOBUFFER;
using namespace Eigen;
using namespace std;
using namespace QtConcurrent;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

calcMetric::calcMetric()
{
    m_iListLength = 10;
    m_iKurtosisHistoryPosition = 0;
    m_iP2PHistoryPosition = 0;
    m_iFuzzyEnHistoryPosition = 0;
    m_iFuzzyEnStart = 0;
    m_iFuzzyEnStep = 10;
    setNewP2P = false;
    setNewFuzzyEn = false;
    setNewKurtosis = false;
    historyReady=false;

}

QPair<RowVectorXd, QPair<QList<double>, int>> createInputList(RowVectorXd input, int dim, double r, double n, double mean, double stdDev)
{
    QPair<RowVectorXd, QPair<QList<double>, int>> funcInput;
    funcInput.first = input;
    QPair<QList<double>, int> inputValues;
    QList<double> doubleInputValues;
    doubleInputValues << mean;
    doubleInputValues << stdDev;
    doubleInputValues << r;
    doubleInputValues << n;
    inputValues.first = doubleInputValues;
    inputValues.second = dim;
    funcInput.second = inputValues;
    return funcInput;

}

double calcFuzzyEn(QPair<RowVectorXd, QPair<QList<double>, int>> input)//RowVectorXd data, double mean, double stdDev, int dim, double r, double n)
{
    RowVectorXd data = input.first;
    QPair<QList<double>, int> inputValues = input.second;
    QList<double> doubleInputValues= inputValues.first;
    int dim = inputValues.second;
    double mean = doubleInputValues[0];
    double stdDev = doubleInputValues[1];
    double r = doubleInputValues[2];
    double n = doubleInputValues[3];

    int length = data.cols();

    double fuzzyEn;


    ArrayXd arDataNorm = (data.array()- mean).array()/(stdDev);
    VectorXd dataNorm = arDataNorm;

        //cout << "mean: " << "\n" << m_dvecMean(k) << "\n" << "\n";

        //cout << "std: " << "\n" << m_dvecStdDev(k) << "\n" << "\n";

        //cout << "DataNorm: " << "\n" << dataNorm << "\n" << "\n";

    Vector2d phi;

    for(int j=0; j<2; j++)
    {
        int m = dim+j;
        MatrixXd patterns;
        patterns.resize(m, length-m+1);

        if (m==1)
            patterns = dataNorm;
        else
        {
            for(int i=0; i<m; i++)
                patterns.row(i)=dataNorm.segment(i,length-m+1);

        }

            //cout << "patterns: \n" << patterns << "\n" << "\n";

        VectorXd patternsMean = patterns.colwise().mean();
        patterns = patterns.rowwise() - patternsMean.transpose(); //check whether correct

            //cout << "patternsMean: \n" << patterns << "\n" << "\n";

        VectorXd aux;
        aux.resize(length-m+1);

        for (int i = 0; i< length-m+1; i++)
        {
            MatrixXd column;
            column.resize(patterns.rows(),patterns.cols());
            for (int l= 0; l < patterns.cols(); l++)
                column.col(l) = patterns.col(i);

            VectorXd distance = ((patterns.array()-column.array()).abs()).colwise().maxCoeff();
                //cout <<  "dist " << i << ": " << distance << "\n";
            VectorXd similarty = (((-1)*(distance.array().pow(n)))/r).exp();
                //cout << "simi " << i << ": " << similarty << "\n";

            aux(i) = ((similarty.sum()-1)/(length-m-1));

        }

            //cout << "aux: " << "\n" << aux <<  "\n";

        phi[j] = (aux.sum()/(length-m));

    }

    fuzzyEn = log(phi[0])-log(phi[1]);


    return fuzzyEn;

}

VectorXd calcMetric::getFuzzyEn()
{  
    return m_dvecFuzzyEn;
}

VectorXd calcMetric::getP2P()
{
    return m_dvecP2P;
}

VectorXd calcMetric::getKurtosis()
{
    return m_dvecKurtosis;
}

MatrixXd calcMetric::getFuzzyEnHistory()
{
    return m_dmatFuzzyEnHistory;
}


MatrixXd calcMetric::getKurtosisHistory()
{
    return m_dmatKurtosisHistory;
}


MatrixXd calcMetric::getP2PHistory()
{
    return m_dmatP2PHistory;
}


void calcMetric::setData(Eigen::MatrixXd input)
{
    m_dmatData = input;
    m_iDataLength = m_dmatData.cols();
    if (m_iChannelCount != m_dmatData.rows())
    {
        m_iChannelCount = m_dmatData.rows();
        m_dvecStdDev.resize(m_iChannelCount);
        m_dvecKurtosis.resize(m_iChannelCount);
        m_dvecMean.resize(m_iChannelCount);
        m_dvecFuzzyEn.resize(m_iChannelCount);
        m_dvecP2P.resize(m_iChannelCount);
        m_dmatFuzzyEnHistory.resize(m_iChannelCount, m_iListLength);
        m_dmatKurtosisHistory.resize(m_iChannelCount, m_iListLength);
        m_dmatP2PHistory.resize(m_iChannelCount, m_iListLength);
    }
}

VectorXd calcMetric::onSeizureDetection(int dim, double r, double n, QList<int> checkChs)
{
    qSort(fuzzyEnUsedChs);
    QList<QPair<RowVectorXd, QPair<QList<double>, int>>> inputList;

    for (int i = 0; i < checkChs.length(); i++)
    {
        if (!fuzzyEnUsedChs.contains(checkChs[i]))
        {
            QPair<RowVectorXd, QPair<QList<double>, int>> funcInput = createInputList(m_dmatData.row(checkChs[i]), dim, r, n, m_dvecMean(checkChs[i]), m_dvecStdDev(checkChs[i]));
            inputList << funcInput;
        }
    }

    QList<double> fuzzyEnResults;
    QFuture<double> fuzzyEnFuture = mapped(inputList, calcFuzzyEn);
    fuzzyEnFuture.waitForFinished();
    QFutureIterator<double> futureResult(fuzzyEnFuture);
    while (futureResult.hasNext())
        fuzzyEnResults << futureResult.next();

    int j = 0;

    for (int i = 0; i < checkChs.length(); i++)
    {
        if (!fuzzyEnUsedChs.contains(checkChs[i]))
        {
            m_dvecFuzzyEn(checkChs[i]) = fuzzyEnResults[j];
            j++;
        }
    }


    return m_dvecFuzzyEn;
}

VectorXd calcP2P(MatrixXd data)
{
    VectorXd max = data.rowwise().maxCoeff();
    VectorXd min = data.rowwise().minCoeff();
    VectorXd P2P = max-min;
    return P2P;
}


void calcMetric::calcP2P()
{

    if (setNewP2P)
    {
        m_dmatP2PHistory.col(m_iP2PHistoryPosition) = m_dvecP2P;
        setNewP2P = false;
        m_iP2PHistoryPosition++;
        if (m_iP2PHistoryPosition>(m_iListLength-1))
        {
            m_iP2PHistoryPosition = 0;
        }
    }

    VectorXd max = m_dmatData.rowwise().maxCoeff();
    VectorXd min = m_dmatData.rowwise().minCoeff();
    m_dvecP2P = max-min;
    setNewP2P = true;
}

QPair<VectorXd, VectorXd> calcKurtosis(MatrixXd data, VectorXd mean, int start, int end)
{
    if (end > data.rows())
        end=data.rows();

    double sum;
    int length = data.cols();
    VectorXd stdDev;
    VectorXd kurtosis;

    for(int i=start; i < end; i++)
    {
        for(int j=0; j < length; j++)
        {
            if (j==0)
            {
                sum = (data(i,j)-mean(i));
                stdDev(i) = pow(sum, 2);
                kurtosis(i) = pow(sum, 4);
            }
            else
            {
                sum = data(i,j) - mean(i);
                stdDev(i) = stdDev(i) + pow(sum, 2);
                kurtosis(i) = kurtosis(i) + pow(sum,4);
            }
        }


        stdDev(i) = stdDev(i)/(length-1);
        stdDev(i) = sqrt(stdDev(i));
        kurtosis(i)=(kurtosis(i))/((length)*pow((stdDev(i)*sqrt(length-1))/sqrt(length),4));

    }

    QPair <VectorXd, VectorXd> outputPair;
    outputPair.first = kurtosis;
    outputPair.second = stdDev;
    return outputPair;


}

void calcMetric::calcKurtosis(int start, int end)
{
    if (end > m_iChannelCount)
        end=m_iChannelCount;

    if (setNewKurtosis)
    {
        m_dmatKurtosisHistory.col(m_iKurtosisHistoryPosition) = m_dvecKurtosis;
        setNewKurtosis = false;
        m_iKurtosisHistoryPosition++;
        if (m_iKurtosisHistoryPosition>(m_iListLength-1))
            m_iKurtosisHistoryPosition = 0;
    }

    m_dvecMean = m_dmatData.rowwise().mean();
    double sum;

    for(int i=start; i < end; i++)
    {        
        for(int j=0; j < m_iDataLength; j++)
        {
            if (j==0)
            {
                sum = (m_dmatData(i,j)-m_dvecMean(i));
                m_dvecStdDev(i) = pow(sum, 2);
                m_dvecKurtosis(i) = pow(sum, 4);
            }
            else
            {
                sum = m_dmatData(i,j) - m_dvecMean(i);
                m_dvecStdDev(i) = m_dvecStdDev(i) + pow(sum, 2);
                m_dvecKurtosis(i) = m_dvecKurtosis(i) + pow(sum,4);
            }
        }


        m_dvecStdDev(i) = m_dvecStdDev(i)/(m_iDataLength-1);
        m_dvecStdDev(i) = sqrt(m_dvecStdDev(i));
        m_dvecKurtosis(i)=(m_dvecKurtosis(i))/((m_iDataLength)*pow((m_dvecStdDev(i)*sqrt(m_iDataLength-1))/sqrt(m_iDataLength),4));

    }

    setNewKurtosis = true;

}



/*
void calcMetric::calcFuzzyEn(int start, int step, int end, int dim, double r, double n)
{
    if (end > m_iChannelCount)
        end = m_iChannelCount;


    if (setNewFuzzyEn) //wrong
    {
        m_dmatFuzzyEnHistory.col(m_iFuzzyEnHistoryPosition) = m_dvecFuzzyEn;
        setNewFuzzyEn = false;
        m_iFuzzyEnHistoryPosition++;
        if (m_iFuzzyEnHistoryPosition>(m_iListLength-1))
            m_iFuzzyEnHistoryPosition = 0;
    }

    //ArrayXd arDataNorm = (m_dmatData-m_dvecMean).array().rowwise()/m_dvecStdDev.transpose().array();
    //MatrixXd dataNorm = arDataNorm;

    for(int k=start; k < end; k=k+step)
    {
        ArrayXd arDataNorm = (m_dmatData.array().row(k)- m_dvecMean(k)).array()/(m_dvecStdDev(k));
        VectorXd dataNorm = arDataNorm;

        //cout << "mean: " << "\n" << m_dvecMean(k) << "\n" << "\n";

        //cout << "std: " << "\n" << m_dvecStdDev(k) << "\n" << "\n";

        //cout << "DataNorm: " << "\n" << dataNorm << "\n" << "\n";

        Vector2d phi;
        for(int j=0; j<2; j++)
        {
            int m = dim+j;
            MatrixXd patterns;
            patterns.resize(m, m_iDataLength-m+1);

            if (m==1)
                patterns = dataNorm.row(k);
            else
            {
                for(int i=0; i<m; i++)
                {
                    patterns.row(i)=dataNorm.segment(i,m_iDataLength-m+1);
                }
            }

            //cout << "patterns: \n" << patterns << "\n" << "\n";

            VectorXd patternsMean = patterns.colwise().mean();
            patterns = patterns.rowwise() - patternsMean.transpose(); //check whether correct

            //cout << "patternsMean: \n" << patterns << "\n" << "\n";

            VectorXd aux;
            aux.resize(m_iDataLength-m+1);

            for (int i = 0; i< m_iDataLength-m+1; i++)
            {
                MatrixXd column;
                column.resize(patterns.rows(),patterns.cols());
                    for (int l= 0; l < patterns.cols(); l++)
                        column.col(l) = patterns.col(i);

                VectorXd distance = ((patterns.array()-column.array()).abs()).colwise().maxCoeff();
                //cout <<  "dist " << i << ": " << distance << "\n";
                VectorXd similarty = (((-1)*(distance.array().pow(n)))/r).exp();
                //cout << "simi " << i << ": " << similarty << "\n";

                aux(i) = ((similarty.sum()-1)/(m_iDataLength-m-1));

            }

            //cout << "aux: " << "\n" << aux <<  "\n";

            phi[j] = (aux.sum()/(m_iDataLength-m));

        }

        m_dvecFuzzyEn[k] = log(phi[0])-log(phi[1]);
    }

    if (m_iFuzzyEnStart < m_iFuzzyEnStep-1)
        m_iFuzzyEnStart++;
    else
    {
        m_iFuzzyEnStart = 0;
        setNewFuzzyEn = true;
    }

}
*/

void calcMetric::calcAll(Eigen::MatrixXd input, int dim, double r, double n)
{
    this->setData(input);
    this->calcP2P();
    this->calcKurtosis(0,1000);

    fuzzyEnUsedChs.clear();

    if (m_iFuzzyEnStart == m_iFuzzyEnStep-1)
    {
        m_dmatFuzzyEnHistory.col(m_iFuzzyEnHistoryPosition) = m_dvecFuzzyEn;
        if (m_iFuzzyEnHistoryPosition < m_iListLength-1)
            m_iFuzzyEnHistoryPosition++;
        else
        {
            m_iFuzzyEnHistoryPosition = 0;
            historyReady = true;
        }
    }

    QList<QPair<RowVectorXd, QPair<QList<double>, int>>> inputList;

    for (int i = m_iFuzzyEnStart; i< m_iChannelCount; i=i+m_iFuzzyEnStep)
    {
        fuzzyEnUsedChs << i;
        QPair<RowVectorXd, QPair<QList<double>, int>> funcInput = createInputList(input.row(i), dim, r, n, m_dvecMean(i), m_dvecStdDev(i));
        inputList << funcInput;
    }

    QList<double> fuzzyEnResults;
    QFuture<double> fuzzyEnFuture = mapped(inputList, calcFuzzyEn);
    fuzzyEnFuture.waitForFinished();
    QFutureIterator<double> futureResult(fuzzyEnFuture);
    while (futureResult.hasNext())
        fuzzyEnResults << futureResult.next();

    int j = 0;

    for (int i = m_iFuzzyEnStart; i<m_iChannelCount; i=i+m_iFuzzyEnStep)
    {
        m_dvecFuzzyEn(i) = fuzzyEnResults[j];
        j++;
    }

    if (m_iFuzzyEnStart < m_iFuzzyEnStep-1)
        m_iFuzzyEnStart++;
    else
        m_iFuzzyEnStart = 0;






    //thread fuzzyCalc1(&calcMetric::calcFuzzyEn,this, 0,4,dim,r,n);
    //thread fuzzyCalc2(&calcMetric::calcFuzzyEn,this, 1,4,dim,r,n);
    //thread fuzzyCalc3(&calcMetric::calcFuzzyEn,this, 2,4,dim,r,n);
    //thread fuzzyCalc4(&calcMetric::calcFuzzyEn,this, 3,4,dim,r,n);

    //fuzzyCalc1.join();
    //fuzzyCalc2.join();
    //fuzzyCalc3.join();
    //fuzzyCalc4.join();
    //cout << m_iFuzzyEnHistoryPosition;

    //this->calcFuzzyEn(m_iFuzzyEnStart, m_iFuzzyEnStep, dim, r, n);
}
