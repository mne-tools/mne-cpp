//=============================================================================================================
/**
 * @file     spline.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     April, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch. All rights reserved.
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
 * @brief    Spline class declaration
 */

#ifndef SPLINE_H
#define SPLINE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QtCharts/QChart>
#include <QtCharts/QSplineSeries>
#include <QVector3D>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QMouseEvent;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
namespace QtCharts{
    class QLineSeries;
}
#else
class QLineSeries;
#endif


//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/** Histogram display using Qtcharts/QSpline
 *
 * @brief Spline class for histogram display using Qtcharts/QSpline
 */
class DISPSHARED_EXPORT Spline: public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<Spline> SPtr;            /**< Shared pointer type for Spline class. */
    typedef QSharedPointer<const Spline> ConstSPtr; /**< Const shared pointer type for Spline class. */

    //=========================================================================================================
    /**
     * The constructor for Spline
     * @param[in] title     string to specify the title displayed on the histogram, defaults to "Spline Histogram".
     * @param[in] parent    sets the behaviour of Spline as an object, defaults to no parent QWidget.
     */
    Spline(QWidget* parent = 0, const QString& title = "Spline Histogram");

    //=========================================================================================================
    /**
     * creates a qspline chart histogram from 2 vectors: class limits and class frequency
     *
     * @param[in] matClassLimitData      vector input filled with class limits.
     * @param[in] matClassFrequencyData  vector input filled with class frequency to the corresponding class.
     */
    template<typename T>
    void setData(const Eigen::Matrix<T, Eigen::Dynamic, 1>& matClassLimitData,
                 const Eigen::Matrix<int, Eigen::Dynamic, 1>& matClassFrequencyData);
    template<typename T>
    void setData(const Eigen::Matrix<T, 1, Eigen::Dynamic>& matClassLimitData,
                 const Eigen::Matrix<int, 1, Eigen::Dynamic>& matClassFrequencyData);

    //=========================================================================================================
    /**
     * Updates the spline with new data
     *
     * @param[in] matClassLimitData      vector input filled with class limits.
     * @param[in] matClassFrequencyData  vector input filled with class frequency to the corresponding class.
     */
    template<typename T>
    void updatePlot(const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& matClassLimitData,
                    const Eigen::VectorXi& matClassFrequencyData);

    //=========================================================================================================
    /**
     * constructor for mouse press event behaviour to create threshold lines and signal emit
     *
     * @param[in] event      mouse press input.
     */
    void mousePressEvent(QMouseEvent *event);

    //=========================================================================================================
    /**
     * splitCoefficientAndExponent takes in QVector value of coefficient and exponent (example: 1.2e-10) and finds the coefficient (1.2) and the appropriate exponent (-12), normalize the exponents to either the lowest or highest exponent in the list then places the values in two separate QVectors
     *
     * @param[in] matClassLimitData      vector input filled with values of class limits (in coefficient and exponent form).
     * @param[in] iClassCount            user input to determine the amount of classes in the histogram.
     * @param[out] vecCoefficientResults  vector filled with values of coefficient only.
     * @param[out] vecExponentResults     vector filled with values of exponent only.
     */
    template<typename T>
    void splitCoefficientAndExponent(const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& matClassLimitData,
                                     int iClassAmount,
                                     Eigen::VectorXd& vecCoefficientResults,
                                     Eigen::VectorXi& vecExponentValues);

    //=========================================================================================================
    /**
     * setThreshold takes in QVector value from outside sources and create the corresponding lines in the histogram
     *
     * @param[in] vecThresholdValues      QVector3D consisting of 3 values corresponding to the x-axis value of the threshold lines.
     */
    void setThreshold(const QVector3D& vecThresholdValues);

    //=========================================================================================================
    /**
     * getThreshold takes in QVector value from outside sources and create the corresponding lines in the histogram
     *
     * @return      returns QList consisting of QVector3D corresponding to the x-axis value of the threshold lines.
     */
    const QVector3D& getThreshold();

    //=========================================================================================================
    /**
     * correctionDisplayTrueValue takes in QVector value from outside sources and create the necessary adjustment of exponential multiplication with base 10
     *
     * @param[in] vecOriginalValues     QVector3D consisting of 3 original values.
     * @param[in] functionName          Choice between getThreshold or setThreshold.
     * @return     returns QVector3D after necessary adjustment.
     */
    QVector3D correctionDisplayTrueValue(QVector3D vecOriginalValues,
                                         QString functionName);

    //=========================================================================================================
    /**
     * updateColorMap takes in string name of color map and the three threshold lines and creates the color gradient
     *
     * @param[in] colorMap  qstring of the color gradient from user input.
     */
    void setColorMap (const QString &colorMap);

    Eigen::VectorXi m_vecResultExponentValues; /**< Common exponent values for the entire histogram*/
    double          m_dMinAxisX;               /**< Display value of the smallest point of the series in x-axis. */
    double          m_dMaxAxisX;               /**< Display value of the largest point on the series in x-axis. */

protected:
    //=========================================================================================================
    /**
     * updateThreshold takes in string name of threshold and its corresponding Qlineseries and creates the line in the QChart
     *
     * @param[in] cThresholdName    name of the Line.
     * @param[in] lineSeries        qlineseries of the corresponding threshold line.
     */
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void updateThreshold (QtCharts::QLineSeries *lineSeries);
    QtCharts::QChart*               m_pChart;            /**< Qchart object that will be shown in the widget. */
    QtCharts::QSplineSeries*        m_pSeries;           /**< Spline data series that will contain the histogram data*/
    QtCharts::QLineSeries*          m_pLeftThreshold;    /**< Vertical line series for the left threshold. */
    QtCharts::QLineSeries*          m_pMiddleThreshold;  /**< Vertical line series for the middle threshold. */
    QtCharts::QLineSeries*          m_pRightThreshold;   /**< Vertical line series for the right threshold. */
#else
    void updateThreshold (QLineSeries *lineSeries);
    QChart*                  m_pChart;            /**< Qchart object that will be shown in the widget. */
    QSplineSeries*           m_pSeries;           /**< Spline data series that will contain the histogram data*/
    QLineSeries*             m_pLeftThreshold;    /**< Vertical line series for the left threshold. */
    QLineSeries*             m_pMiddleThreshold;  /**< Vertical line series for the middle threshold. */
    QLineSeries*             m_pRightThreshold;   /**< Vertical line series for the right threshold. */
#endif
    int                      m_iMaximumFrequency; /**< Highest value of frequency (y-axis). */
    QList<QVector3D>         m_pReturnList;       /**< QList consisting of 2 QVector3D used in getThreshold function*/
    QString                  m_colorMap;          /**< QString that will be used to set the color mapping on the histogram*/
    QVector3D                m_vecReturnVector;   /**< QVector3D after correction with correctionDisplayTrueValue function*/

signals:
    //=========================================================================================================
    /**
     * emit signal consisting of three threshold lines x-axis value if any one of them is changed
     *
     * @param[out]  leftThreshold      value of the left threshold line (initialized as minAxisX).
     * @param[out]  middleThreshold    value of the middle threshold line (initialized as maxAxisX).
     * @param[out]  rightThreshold     value of the right threshold line (initialized as maxAxisX).
     */
    void borderChanged(double leftThreshold, double middleThreshold, double rightThreshold);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

template <typename T>
void Spline::setData(const Eigen::Matrix<T, Eigen::Dynamic, 1>& matClassLimitData,
                     const Eigen::Matrix<int, Eigen::Dynamic, 1>& matClassFrequencyData)
{
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> matrixName(matClassLimitData.rows(),1);
    matrixName.col(0) = matClassLimitData;
    this->updatePlot(matrixName, matClassFrequencyData);
}

//=========================================================================================================

template <typename T>
void Spline::setData(const Eigen::Matrix<T, 1, Eigen::Dynamic>& matClassLimitData,
                     const Eigen::Matrix<int, 1, Eigen::Dynamic>& matClassFrequencyData)
{
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> matrixName(1, matClassLimitData.cols());
    matrixName.row(0) = matClassLimitData;
    this->updatePlot(matrixName, matClassFrequencyData);
}

//=========================================================================================================

template<typename T>
void Spline::updatePlot(const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& matClassLimitData,
                        const Eigen::VectorXi& matClassFrequencyData)
{
    Eigen::VectorXd resultDisplayValues;
    int iClassAmount = matClassFrequencyData.rows();
    this->splitCoefficientAndExponent (matClassLimitData, iClassAmount, resultDisplayValues, m_vecResultExponentValues);

    //  Start of Qtchart histogram display
    QString histogramExponent;
    histogramExponent = "X-axis scale: 10e" + QString::number(m_vecResultExponentValues(0));   //used to tell the user the exponential scale used in the histogram
    m_pSeries->setName(histogramExponent);
    m_pSeries->clear();
    m_pChart->removeSeries(m_pSeries);
    m_pChart->removeSeries(m_pLeftThreshold);              //create new series and then clear the plot and update with new data
    m_pChart->removeSeries(m_pMiddleThreshold);
    m_pChart->removeSeries(m_pRightThreshold);

    m_dMinAxisX = resultDisplayValues(0);
    m_dMaxAxisX = resultDisplayValues(iClassAmount);
    double classMark;                         //class mark is the middle point between lower and upper class limit
    m_iMaximumFrequency = 0;                    //iMaximumFrequency used to create an intuitive histogram

    for (int ir = 0; ir < iClassAmount; ++ir)
    {
        classMark = (resultDisplayValues(ir) + resultDisplayValues(ir+1))/2 ;
        m_pSeries->append(classMark, matClassFrequencyData(ir));
        //std::cout << "Spline data points = " << classMark << ", " << matClassFrequencyData(ir) << std::endl;
        if (matClassFrequencyData(ir) > m_iMaximumFrequency)
        {
            m_iMaximumFrequency = matClassFrequencyData(ir);
        }
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    m_pLeftThreshold = new QtCharts::QLineSeries();
    m_pMiddleThreshold = new QtCharts::QLineSeries();
    m_pRightThreshold = new QtCharts::QLineSeries();
#else
    m_pLeftThreshold = new QLineSeries();
    m_pMiddleThreshold = new QLineSeries();
    m_pRightThreshold = new QLineSeries();
#endif

    m_pLeftThreshold->setName("left");
    m_pMiddleThreshold->setName("middle");
    m_pRightThreshold->setName("right");

    m_pChart->addSeries(m_pSeries);
    m_pChart->legend()->setVisible(true);
    m_pChart->legend()->setAlignment(Qt::AlignBottom);
    m_pChart->createDefaultAxes();
    m_pChart->axisX()->setRange(m_dMinAxisX, m_dMaxAxisX);
}

//=============================================================================================================

template <typename T>
void Spline::splitCoefficientAndExponent (const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& matClassLimitData,
                                          int iClassAmount,
                                          Eigen::VectorXd& vecCoefficientResults,
                                          Eigen::VectorXi& vecExponentValues)
{
    vecCoefficientResults.resize(iClassAmount + 1);
    vecExponentValues.resize(iClassAmount + 1);
    double originalValue(0.0),
         limitDisplayValue(0.0),
         doubleExponentValue(0.0);
    int    limitExponentValue(0);
    for (int ir = 0; ir <= iClassAmount; ++ir)
    {
        originalValue = matClassLimitData(ir);
        if (originalValue == 0.0)                          //mechanism to guard against evaluation of log(0.0) which is infinity
        {
            doubleExponentValue = 0.0;
        }

        else
        {
            doubleExponentValue = log10(std::fabs(originalValue));                    //return the exponent value in double
        }

        limitExponentValue = round(doubleExponentValue);                        //round the exponent value to the nearest signed integer
        limitDisplayValue = originalValue * (pow(10,-(limitExponentValue)));    //display value is derived from multiplying class limit with inverse 10 to the power of negative exponent
        vecCoefficientResults(ir) = limitDisplayValue;                          //append the display value to the return vector
        vecExponentValues(ir) = limitExponentValue;                             //append the exponent value to the return vector
    }

    int lowestExponentValue{0},
    highestExponentValue{0};

    for (int ir = 0; ir <= iClassAmount; ++ir)
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

    if (highestExponentValue > 0)
    {
        for (int ir = 0; ir <= iClassAmount; ++ir)
        {
            while (vecExponentValues(ir) < highestExponentValue)     //normalize the values by multiplying the display value by 10 and reducing the exponentValue by 1 until exponentValue reach the lowestExponentValue
            {
                vecCoefficientResults(ir) = vecCoefficientResults(ir) / 10;
                vecExponentValues(ir)++;
            }
        }
    }

    if (lowestExponentValue < 0)
    {
        for (int ir = 0; ir <= iClassAmount; ++ir)
        {
            while (vecExponentValues(ir) > lowestExponentValue)
            {
                vecCoefficientResults(ir) = vecCoefficientResults(ir) * 10;
                vecExponentValues(ir)--;
            }
        }
    }
}
} // NAMESPACE

#endif // SPLINE_H
