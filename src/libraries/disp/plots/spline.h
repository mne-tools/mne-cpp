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
#include <QVector3D>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QMouseEvent;

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/** Histogram display using QPainter with spline interpolation
 *
 * @brief Spline class for histogram display using QPainter
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
     * @param[in] parent    sets the behaviour of Spline as an object, defaults to no parent QWidget.
     * @param[in] title     string to specify the title displayed on the histogram, defaults to "Spline Histogram".
     */
    Spline(QWidget* parent = nullptr, const QString& title = "Spline Histogram");

    //=========================================================================================================
    /**
     * creates a spline histogram from 2 vectors: class limits and class frequency
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
    void mousePressEvent(QMouseEvent *event) override;

    //=========================================================================================================
    /**
     * splitCoefficientAndExponent takes in QVector value of coefficient and exponent
     * and normalizes them.
     *
     * @param[in] matClassLimitData      vector input filled with values of class limits.
     * @param[in] iClassAmount           amount of classes in the histogram.
     * @param[out] vecCoefficientResults  vector filled with values of coefficient only.
     * @param[out] vecExponentValues      vector filled with values of exponent only.
     */
    template<typename T>
    void splitCoefficientAndExponent(const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& matClassLimitData,
                                     int iClassAmount,
                                     Eigen::VectorXd& vecCoefficientResults,
                                     Eigen::VectorXi& vecExponentValues);

    //=========================================================================================================
    /**
     * setThreshold takes in QVector3D value and creates the corresponding lines in the histogram
     *
     * @param[in] vecThresholdValues      QVector3D consisting of 3 values corresponding to the threshold lines.
     */
    void setThreshold(const QVector3D& vecThresholdValues);

    //=========================================================================================================
    /**
     * getThreshold retrieves the current threshold values
     *
     * @return      returns QVector3D corresponding to the x-axis value of the threshold lines.
     */
    const QVector3D& getThreshold();

    //=========================================================================================================
    /**
     * correctionDisplayTrueValue creates necessary adjustment of exponential multiplication with base 10
     *
     * @param[in] vecOriginalValues     QVector3D consisting of 3 original values.
     * @param[in] upOrDown              Choice between "up" or "down" conversion direction.
     * @return     returns QVector3D after necessary adjustment.
     */
    QVector3D correctionDisplayTrueValue(QVector3D vecOriginalValues,
                                         QString upOrDown);

    //=========================================================================================================
    /**
     * setColorMap sets the color mapping on the histogram background
     *
     * @param[in] colorMap  qstring of the color gradient from user input.
     */
    void setColorMap(const QString &colorMap);

    Eigen::VectorXi m_vecResultExponentValues; /**< Common exponent values for the entire histogram*/
    double          m_dMinAxisX;               /**< Display value of the smallest point of the series in x-axis. */
    double          m_dMaxAxisX;               /**< Display value of the largest point on the series in x-axis. */

protected:
    //=========================================================================================================
    /**
     * Paints the spline chart.
     *
     * @param[in] event  The paint event.
     */
    void paintEvent(QPaintEvent *event) override;

    //=========================================================================================================
    /**
     * Converts data X coordinate to pixel X coordinate.
     */
    double dataToPixelX(double dataX) const;

    //=========================================================================================================
    /**
     * Converts data Y coordinate to pixel Y coordinate.
     */
    double dataToPixelY(double dataY) const;

    //=========================================================================================================
    /**
     * Converts pixel X coordinate to data X coordinate.
     */
    double pixelToDataX(double pixelX) const;

    //=========================================================================================================
    /**
     * Returns the left margin in pixels.
     */
    int leftMargin() const { return 60; }

    //=========================================================================================================
    /**
     * Returns the right margin in pixels.
     */
    int rightMargin() const { return 20; }

    //=========================================================================================================
    /**
     * Returns the top margin in pixels.
     */
    int topMargin() const { return 30; }

    //=========================================================================================================
    /**
     * Returns the bottom margin in pixels.
     */
    int bottomMargin() const { return 40; }

    QList<QPointF>  m_seriesData;           /**< Spline data points (classMark, frequency). */
    double          m_dLeftThreshold;       /**< X-axis value of the left threshold line. */
    double          m_dMiddleThreshold;     /**< X-axis value of the middle threshold line. */
    double          m_dRightThreshold;      /**< X-axis value of the right threshold line. */
    bool            m_bHasData;             /**< Whether data has been loaded. */
    bool            m_bHasThresholds;       /**< Whether thresholds have been set. */
    int             m_iMaximumFrequency;    /**< Highest value of frequency (y-axis). */
    QString         m_colorMap;             /**< Color map name for background gradient. */
    QVector3D       m_vecReturnVector;      /**< Cached return value for getThreshold. */

signals:
    //=========================================================================================================
    /**
     * emit signal consisting of three threshold lines x-axis value if any one of them is changed
     *
     * @param[out]  leftThreshold      value of the left threshold line.
     * @param[out]  middleThreshold    value of the middle threshold line.
     * @param[out]  rightThreshold     value of the right threshold line.
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
    this->splitCoefficientAndExponent(matClassLimitData, iClassAmount, resultDisplayValues, m_vecResultExponentValues);

    m_dMinAxisX = resultDisplayValues(0);
    m_dMaxAxisX = resultDisplayValues(iClassAmount);

    m_seriesData.clear();
    m_iMaximumFrequency = 0;

    for (int ir = 0; ir < iClassAmount; ++ir)
    {
        double classMark = (resultDisplayValues(ir) + resultDisplayValues(ir + 1)) / 2.0;
        m_seriesData.append(QPointF(classMark, matClassFrequencyData(ir)));
        if (matClassFrequencyData(ir) > m_iMaximumFrequency)
        {
            m_iMaximumFrequency = matClassFrequencyData(ir);
        }
    }

    m_bHasData = true;
    m_dLeftThreshold = m_dMinAxisX;
    m_dMiddleThreshold = m_dMaxAxisX;
    m_dRightThreshold = m_dMaxAxisX;
    m_bHasThresholds = false;

    QWidget::update();
}

//=============================================================================================================

template <typename T>
void Spline::splitCoefficientAndExponent(const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& matClassLimitData,
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
        if (originalValue == 0.0)
        {
            doubleExponentValue = 0.0;
        }
        else
        {
            doubleExponentValue = log10(std::fabs(originalValue));
        }

        limitExponentValue = round(doubleExponentValue);
        limitDisplayValue = originalValue * (pow(10, -(limitExponentValue)));
        vecCoefficientResults(ir) = limitDisplayValue;
        vecExponentValues(ir) = limitExponentValue;
    }

    int lowestExponentValue{0},
    highestExponentValue{0};

    for (int ir = 0; ir <= iClassAmount; ++ir)
    {
        if (vecExponentValues(ir) < lowestExponentValue)
        {
            lowestExponentValue = vecExponentValues(ir);
        }
        if (vecExponentValues(ir) > highestExponentValue)
        {
            highestExponentValue = vecExponentValues(ir);
        }
    }

    if (highestExponentValue > 0)
    {
        for (int ir = 0; ir <= iClassAmount; ++ir)
        {
            while (vecExponentValues(ir) < highestExponentValue)
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
