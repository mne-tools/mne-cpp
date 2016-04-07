//=============================================================================================================
//=============================================================================================================

/**
* @file     bar.cpp
* @author   Ricky Tjen <ricky270@student.sgu.ac.id>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     April, 2016
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
* @brief    implementation of bar class
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bar.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

 Bar::Bar(const VectorXd& matClassLimitData, const VectorXi& matClassFrequencyData, int iClassAmount, int iPrecisionValue)
{
    Eigen::VectorXd resultDisplayValues;
    Eigen::VectorXi resultExponentValues;
    splitCoefficientAndExponent (matClassLimitData, iClassAmount, resultDisplayValues, resultExponentValues);

    //  Start of Qtchart histogram display
    QString histogramExponent;
    histogramExponent = "X-axis scale: 10e" + QString::number(resultExponentValues(0));
    QBarSet *set = new QBarSet(histogramExponent);
    QStringList categories;
    QString currentLimits;
    int classFreq;

    for (int kr=0; kr < iClassAmount; kr++)
    {
        classFreq = matClassFrequencyData(kr);
        currentLimits = ((QString::number(resultDisplayValues(kr), 'g' ,iPrecisionValue) + " to " + (QString::number(resultDisplayValues(kr+1), 'g', iPrecisionValue))));
        categories << currentLimits;
        *set << classFreq;
    }

    //  Start of Qtchart histogram display
    QBarSeries *series = new QBarSeries();
    series->append(set);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("MNE-CPP Histogram Example");
    chart->setAnimationOptions(QChart::SeriesAnimations);

    QBarCategoryAxis *axis = new QBarCategoryAxis();
    axis->append(categories);
    chart->createDefaultAxes();
    chart->setAxisX(axis, series);

    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    QWidget* widget = new QWidget();
    QGridLayout* layout = new QGridLayout();

    layout->addWidget(chartView,0,0);

    widget->setLayout(layout);
    widget->resize(420, 300);
    widget->show();
}



//*************************************************************************************************************

void Bar::splitCoefficientAndExponent (const Eigen::VectorXd& matClassLimitData, int iClassAmount, Eigen::VectorXd& vecCoefficientResults, Eigen::VectorXi& vecExponentValues)
{
    vecCoefficientResults.resize(iClassAmount + 1);
    vecExponentValues.resize(iClassAmount + 1);
    double originalValue(0.0),
           limitDisplayValue(0.0),
           doubleExponentValue(0.0);
    int    limitExponentValue(0);
    for (int ir=0; ir <= iClassAmount; ir++)
    {
        originalValue = matClassLimitData(ir);
        if (originalValue == 0.0)                          //mechanism to guard against evaluation of log(0.0) which is infinity
        {
            doubleExponentValue = 0.0;
        }
        else
        {
            doubleExponentValue = log10(abs(originalValue));                    //return the exponent value in double
        }
        limitExponentValue = round(doubleExponentValue);                        //round the exponent value to the nearest signed integer
        limitDisplayValue = originalValue * (pow(10,-(limitExponentValue)));    //display value is derived from multiplying class limit with inverse 10 to the power of negative exponent
        vecCoefficientResults(ir) = limitDisplayValue;                         //append the display value to the return vector
        vecExponentValues(ir) = limitExponentValue;                            //append the exponent value to the return vector
    }

    int lowestExponentValue{0},
        highestExponentValue{0};
    for (int ir=0; ir <= iClassAmount; ir++)
    {
        if (vecExponentValues(ir) < lowestExponentValue)
        {
            lowestExponentValue = vecExponentValues(ir);     //find lowest exponent value to normalize display values for negative exponent
        }
        if (vecExponentValues(ir) > highestExponentValue)       //find highest exponent value to normalize display values for positive exponent
        {
            highestExponentValue = vecExponentValues(ir);
        }
    }

    if (highestExponentValue == 0)
    {
        for (int ir=0; ir <= iClassAmount; ir++)
        {
            while (vecExponentValues(ir) > lowestExponentValue)     //normalize the values by multiplying the display value by 10 and reducing the exponentValue by 1 until exponentValue reach the lowestExponentValue
            {
                vecCoefficientResults(ir) = vecCoefficientResults(ir) * 10;
                vecExponentValues(ir)--;
            }
        }
    }
    if (lowestExponentValue == 0)
    {
        for (int ir=0; ir <= iClassAmount; ir++)
        {
            while (vecExponentValues(ir) < highestExponentValue)
            {
                vecCoefficientResults(ir) = vecCoefficientResults(ir) / 10;
                vecExponentValues(ir)++;
            }
        }
    }
}



//*************************************************************************************************************
