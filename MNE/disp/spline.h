//=============================================================================================================
/**
* @file     spline.h
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
* @brief    spline class declaration
*/

#ifndef SPLINE_H
#define SPLINE_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "disp_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QString>
#include <QGridLayout>
#include <QSharedPointer>
#include <QMainWindow>
#include <QtCharts/QChartView>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QBarCategoryAxis>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QGraphicsScene>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QSplineSeries>
#include <QtWidgets/QGraphicsTextItem>
#include <QtGui/QMouseEvent>
#include <QDebug>
#include <QtCharts/QChartGlobal>
#include <QtCharts/QLegendMarker>
#include <QtCharts/QLegend>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISPLIB
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
QT_CHARTS_USE_NAMESPACE


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

/** histogram display using Qtcharts/QSpline
*
* @brief Spline class for histogram display using Qtcharts/QSpline
*/

class DISPSHARED_EXPORT Spline: public QWidget
{
    Q_OBJECT
public:

    //=========================================================================================================
    /**
    * The constructor for Spline
    */
    Spline(const QString& title = "Spline Histogram", QWidget* parent = 0);

    //=========================================================================================================
    /**
    * creates a qspline chart histogram from 2 vectors: class limits and class frequency
    *
    * @param[in]  matClassLimitData      vector input filled with class limits
    * @param[in]  matClassFrequencyData  vector input filled with class frequency to the corresponding class
    * @param[in]  iPrecisionValue        user input to determine the amount of digits of coefficient shown in the bar histogram
    */
    template<typename T>
    void setData(const Eigen::Matrix<T, Eigen::Dynamic, 1>& matClassLimitData, const Eigen::Matrix<int, Eigen::Dynamic, 1>& matClassFrequencyData, int iPrecisionValue);
    template<typename T>
    void setData(const Eigen::Matrix<T, 1, Eigen::Dynamic>& matClassLimitData, const Eigen::Matrix<int, 1, Eigen::Dynamic>& matClassFrequencyData, int iPrecisionValue);

    //=========================================================================================================
    /**
    * constructor for mouse press event behaviour to create threshold lines and signal emit
    *
    * @param[in]  event      mouse press input
    */
    void Spline::mousePressEvent(QMouseEvent *event);

    //=========================================================================================================
    /**
    * Updates the spline with new data
    *
    * @param[in]  matClassLimitData      vector input filled with class limits
    * @param[in]  matClassFrequencyData  vector input filled with class frequency to the corresponding class
    * @param[in]  iPrecisionValue        user input to determine the amount of digits of coefficient shown in the bar histogram
    */
    template<typename T>
    void updatePlot(const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& matClassLimitData, const Eigen::VectorXi& matClassFrequencyData, int iPrecisionValue);

    //=========================================================================================================
    /**
    * splitCoefficientAndExponent takes in QVector value of coefficient and exponent (example: 1.2e-10) and finds the coefficient (1.2) and the appropriate exponent (-12), normalize the exponents to either the lowest or highest exponent in the list then places the values in two separate QVectors
    *
    * @param[in]  matClassLimitData      vector input filled with values of class limits (in coefficient and exponent form)
    * @param[in]  iClassCount            user input to determine the amount of classes in the histogram
    * @param[out] vecCoefficientResults  vector filled with values of coefficient only
    * @param[out] vecExponentResults     vector filled with values of exponent only
    */
    template<typename T>
    void splitCoefficientAndExponent (const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& matClassLimitData, int iClassAmount, Eigen::VectorXd& vecCoefficientResults, Eigen::VectorXi& vecExponentValues);

    private:
        QChart                  *m_pChart;          //Qchart object that will be shown in the widget
        QLineSeries             *leftThreshold;     //Vertical line series for the left threshold
        QLineSeries             *middleThreshold;   //Vertical line series for the middle threshold
        QLineSeries             *rightThreshold;    //Vertical line series for the right threshold
        double minAxisX,                            //Value of the smallest point of the series in x-axis
               maxAxisX;                            //Value of the largest point on the series in x-axis
        int maximumFrequency;                       //Highest value of frequency (y-axis)
        QLegendMarker *marker;                      //Variable to specify the legend of the threshold lines
        Eigen::VectorXi resultExponentValues;       //Common exponent values for the entire

signals:
    //=========================================================================================================
    /**
    * emit signal consisting of three threshold lines x-axis value if any one of them is changed
    *
    * @param[out]  leftThreshold      value of the left threshold line (initialized as minAxisX)
    * @param[out]  middleThreshold    value of the middle threshold line (initialized as maxAxisX)
    * @param[out]  rightThreshold     value of the right threshold line (initialized as maxAxisX)
    */
    void borderChanged(double leftThreshold, double middleThreshold, double rightThreshold);
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

template <typename T>
void Spline::setData(const Eigen::Matrix<T, Eigen::Dynamic, 1>& matClassLimitData, const Eigen::Matrix<int, Eigen::Dynamic, 1>& matClassFrequencyData, int iPrecisionValue)
{
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> matrixName(matClassLimitData.rows(),1);
    matrixName.col(0)= matClassLimitData;
    this->updatePlot(matrixName, matClassFrequencyData, iPrecisionValue);
}


//=========================================================================================================

template <typename T>
void Spline::setData(const Eigen::Matrix<T, 1, Eigen::Dynamic>& matClassLimitData, const Eigen::Matrix<int, 1, Eigen::Dynamic>& matClassFrequencyData, int iPrecisionValue)
{
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> matrixName(1, matClassLimitData.cols());
    matrixName.row(0)= matClassLimitData;
    this->updatePlot(matrixName, matClassFrequencyData, iPrecisionValue);
}


//=========================================================================================================

template<typename T>
void Spline::updatePlot(const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& matClassLimitData, const Eigen::VectorXi& matClassFrequencyData, int iPrecisionValue)
{
    Eigen::VectorXd resultDisplayValues;
    int iClassAmount = matClassFrequencyData.rows();
    this->splitCoefficientAndExponent (matClassLimitData, iClassAmount, resultDisplayValues, resultExponentValues);

    //  Start of Qtchart histogram display
    QString histogramExponent;
    histogramExponent = "X-axis scale: 10e" + QString::number(resultExponentValues(0));   //used to tell the user the exponential scale used in the histogram

    QSplineSeries *series = new QSplineSeries();
    QSplineSeries *shadowSeries = new QSplineSeries();
    QSplineSeries *shadowSeriesY = new QSplineSeries();

    series->setName(histogramExponent);

    minAxisX = resultDisplayValues(0);
    maxAxisX = resultDisplayValues(iClassAmount);
    double classMark;                         //class mark is the middle point between lower and upper class limit
    maximumFrequency = 0;                     //maximumFrequency used to create an intuitive histogram

    for (int ir=0; ir < iClassAmount; ++ir)
    {
        classMark = (resultDisplayValues(ir) + resultDisplayValues(ir+1))/2;
        series->append(classMark, matClassFrequencyData(ir));
        shadowSeries->append(classMark, 0);
        shadowSeriesY->append(minAxisX, matClassFrequencyData(ir));
        std::cout << "Spline data points = " << classMark << ", " << matClassFrequencyData(ir) << std::endl;
        if (matClassFrequencyData(ir) > maximumFrequency)
        {
            maximumFrequency = matClassFrequencyData(ir);
        }
    }

    m_pChart->removeAllSeries();              //create new series and then clear the plot and update with new data
    m_pChart->addSeries(series);

    leftThreshold = new QLineSeries();
    middleThreshold = new QLineSeries();
    rightThreshold = new QLineSeries();
    leftThreshold->append(minAxisX, 0);       //initialize threshold lines
    middleThreshold->append(maxAxisX, 0);
    rightThreshold->append(maxAxisX, 0);
    leftThreshold->setVisible(false);         //threshold lines intially invisible
    middleThreshold->setVisible(false);
    rightThreshold->setVisible(false);

    m_pChart->legend()->setVisible(true);
    m_pChart->legend()->setAlignment(Qt::AlignBottom);
    m_pChart->addSeries(leftThreshold);
    m_pChart->addSeries(middleThreshold);
    m_pChart->addSeries(rightThreshold);
    m_pChart->createDefaultAxes();
    m_pChart->axisX()->setRange(minAxisX, maxAxisX);
    m_pChart->axisY()->setRange(0,maximumFrequency);
}


//*************************************************************************************************************

template <typename T>
void Spline::splitCoefficientAndExponent (const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& matClassLimitData, int iClassAmount, Eigen::VectorXd& vecCoefficientResults, Eigen::VectorXi& vecExponentValues)
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
        vecCoefficientResults(ir) = limitDisplayValue;                          //append the display value to the return vector
        vecExponentValues(ir) = limitExponentValue;                             //append the exponent value to the return vector
    }

    int lowestExponentValue{0},
    highestExponentValue{0};

    for (int ir=0; ir <= iClassAmount; ir++)
    {
        if (vecExponentValues(ir) < lowestExponentValue)
        {
            lowestExponentValue = vecExponentValues(ir);        //find lowest exponent value to normalize display values for negative exponent
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
}

//=========================================================================================================
// NAMESPACE

#endif // SPLINE_H
