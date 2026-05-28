//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Gabriel B Motta <gbmotta@mgh.harvard.edu>
 *   Andreas Griesshammer <ag@fieldlineinc.com>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file bar.h
 * @since 2022
 * @date  April 2026
 * @brief QPainter-based bar / histogram widget for Eigen class-limit + frequency vectors.
 *
 * Bar renders a MATLAB-style @c bar() chart in pure @c QPainter into a
 * @c QWidget canvas. The data interface is templated on the value type
 * and accepts class limits and per-bin counts as Eigen column or row
 * vectors, together with a precision hint controlling how many
 * significant digits of the limit values are printed below each bin.
 * It is used by histogram-oriented dialogs (e.g. evoked amplitude or
 * noise distributions) where a lightweight, dependency-free histogram
 * is sufficient.
 */

#ifndef BAR_H
#define BAR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QStringList>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * @brief Bar chart / histogram QWidget rendering Eigen class-limit + frequency vectors with QPainter.
 *
 * Templated on the value type so callers can pass either integer or
 * floating bin limits. Use @ref setData to install a new dataset and
 * @ref updatePlot to refresh an existing chart in place; both call
 * @ref splitCoefficientAndExponent internally to format the tick
 * labels with a configurable number of significant digits.
 */
class DISPSHARED_EXPORT Bar : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<Bar> SPtr;            /**< Shared pointer type for Bar class. */
    typedef QSharedPointer<const Bar> ConstSPtr; /**< Const shared pointer type for Bar class. */

    //=========================================================================================================
    /**
     * The constructor for Bar.
     */
    Bar(const QString& title = "", QWidget* parent = nullptr);

    //=========================================================================================================
    /**
     * Sets new data to the bar chart
     *
     * @param[in] matClassLimitData      vector input filled with class limits.
     * @param[in] matClassFrequencyData  vector input filled with class frequency to the corresponding class.
     * @param[in] iPrecisionValue        user input to determine the amount of digits of coefficient shown in the histogram.
     */
    template<typename T>
    void setData(const Eigen::Matrix<T, Eigen::Dynamic, 1>& matClassLimitData,
                 const Eigen::Matrix<int, Eigen::Dynamic, 1>& matClassFrequencyData,
                 int iPrecisionValue);
    template<typename T>
    void setData(const Eigen::Matrix<T, 1, Eigen::Dynamic>& matClassLimitData,
                 const Eigen::Matrix<int, 1, Eigen::Dynamic>& matClassFrequencyData,
                 int iPrecisionValue);

    //=========================================================================================================
    /**
     * Updates the bar plot with the new data
     *
     * @param[in] matClassLimitData      vector input filled with class limits.
     * @param[in] matClassFrequencyData  vector input filled with class frequency to the corresponding class.
     * @param[in] iPrecisionValue        user input to determine the amount of digits of coefficient shown in the histogram.
     */
    template<typename T>
    void updatePlot(const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& matClassLimitData,
                    const Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic>& matClassFrequencyData,
                    int iPrecisionValue);

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

protected:
    //=========================================================================================================
    /**
     * Paints the bar chart.
     *
     * @param[in] event  The paint event.
     */
    void paintEvent(QPaintEvent *event) override;

private:
    QString         m_sTitle;           /**< Chart title. */
    QString         m_sLegend;          /**< Legend text (exponent scale info). */
    QStringList     m_categories;       /**< Category labels for x-axis. */
    QList<int>      m_frequencies;      /**< Frequency values for each bar. */
    int             m_iMaxFrequency;    /**< Maximum frequency value (for y-axis scaling). */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

template<typename T>
void Bar::setData(const Eigen::Matrix<T, Eigen::Dynamic, 1>& matClassLimitData,
                  const Eigen::Matrix<int, Eigen::Dynamic, 1>& matClassFrequencyData,
                  int iPrecisionValue)
{
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> matrixName(matClassLimitData.rows(),1);
    matrixName.col(0) = matClassLimitData;
    this->updatePlot(matrixName, matClassFrequencyData, iPrecisionValue);
}

//=============================================================================================================

template<typename T>
void Bar::setData(const Eigen::Matrix<T, 1, Eigen::Dynamic>& matClassLimitData,
                  const Eigen::Matrix<int, 1, Eigen::Dynamic>& matClassFrequencyData,
                  int iPrecisionValue)
{
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> matrixName(1, matClassLimitData.cols());
    matrixName.row(0) = matClassLimitData;
    this->updatePlot(matrixName, matClassFrequencyData, iPrecisionValue);
}

//=============================================================================================================

template<typename T>
void Bar::updatePlot(const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& matClassLimitData,
                     const Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic>& matClassFrequencyData,
                     int iPrecisionValue)
{
    Eigen::VectorXd resultDisplayValues;
    Eigen::VectorXi resultExponentValues;
    int iClassAmount = std::min(static_cast<int>(matClassFrequencyData.rows()),
                                static_cast<int>(matClassLimitData.rows()) - 1);
    if(iClassAmount < 1) return;
    this->splitCoefficientAndExponent(matClassLimitData, iClassAmount, resultDisplayValues, resultExponentValues);

    // Setup legend text
    m_sLegend = "X-axis scale: 10e" + QString::number(resultExponentValues(0));

    // Build category labels and frequency list
    m_categories.clear();
    m_frequencies.clear();
    m_iMaxFrequency = 0;

    for (int kr = 0; kr < iClassAmount; ++kr)
    {
        int classFreq = matClassFrequencyData(kr);
        QString currentLimits = QString::number(resultDisplayValues(kr), 'g', iPrecisionValue)
                              + " to "
                              + QString::number(resultDisplayValues(kr + 1), 'g', iPrecisionValue);
        m_categories << currentLimits;
        m_frequencies << classFreq;
        if (classFreq > m_iMaxFrequency) {
            m_iMaxFrequency = classFreq;
        }
    }

    QWidget::update();
}

//=============================================================================================================

template <typename T>
void Bar::splitCoefficientAndExponent(const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& matClassLimitData,
                                       int iClassAmount,
                                       Eigen::VectorXd& vecCoefficientResults, Eigen::VectorXi& vecExponentValues)
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

#endif // BAR_H
