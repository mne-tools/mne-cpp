//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     bar.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    QPainter-based bar / histogram widget for Eigen class-limit + frequency vectors.
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
 * @ref updatePlot to refresh an existing chart in place; each bar is
 * labelled with its class limits in data units.
 *
 * @snippet ex_disp_plots/main.cpp bar_usage
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
     *
     * @param[in] title   Title drawn above the bar chart (default empty).
     * @param[in] parent  Parent widget (default nullptr).
     */
    Bar(const QString& title = "", QWidget* parent = nullptr);

    //=========================================================================================================
    /**
     * Sets new data to the bar chart
     *
     * @param[in] matClassLimitData      vector input filled with class limits.
     * @param[in] matClassFrequencyData  vector input filled with class frequency to the corresponding class.
     * @param[in] iPrecisionValue        significant digits of the class limits on the x axis.
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
     * @param[in] iPrecisionValue        significant digits of the class limits on the x axis.
     */
    template<typename T>
    void updatePlot(const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& matClassLimitData,
                    const Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic>& matClassFrequencyData,
                    int iPrecisionValue);

    //=========================================================================================================
    /**
     * Sets the label of the x axis, e.g. the quantity and unit of the class limits.
     *
     * @param[in] label  The x axis label.
     */
    void setXLabel(const QString& label);

    //=========================================================================================================
    /**
     * Sets the label of the y axis.
     *
     * @param[in] label  The y axis label.
     */
    void setYLabel(const QString& label);

protected:
    //=========================================================================================================
    /**
     * Paints the bar chart.
     *
     * @param[in] event  The paint event.
     */
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_sTitle;         /**< Chart title. */
    QString m_sXLabel;        /**< X axis label. */
    QString m_sYLabel;        /**< Y axis label. */
    QStringList m_categories; /**< Category labels for x-axis. */
    QList<int> m_frequencies; /**< Frequency values for each bar. */
    int m_iMaxFrequency;      /**< Maximum frequency value (for y-axis scaling). */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

template<typename T>
void Bar::setData(const Eigen::Matrix<T, Eigen::Dynamic, 1>& matClassLimitData,
                  const Eigen::Matrix<int, Eigen::Dynamic, 1>& matClassFrequencyData,
                  int iPrecisionValue)
{
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> matrixName(matClassLimitData.rows(), 1);
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
    int iClassAmount = std::min(static_cast<int>(matClassFrequencyData.rows()),
                                static_cast<int>(matClassLimitData.rows()) - 1);
    if (iClassAmount < 1)
        return;
    // Build category labels and frequency list
    m_categories.clear();
    m_frequencies.clear();
    m_iMaxFrequency = 0;

    for (int kr = 0; kr < iClassAmount; ++kr) {
        int classFreq = matClassFrequencyData(kr);
        QString currentLimits = QString::number(static_cast<double>(matClassLimitData(kr)), 'g', iPrecisionValue) + " to " + QString::number(static_cast<double>(matClassLimitData(kr + 1)), 'g', iPrecisionValue);
        m_categories << currentLimits;
        m_frequencies << classFreq;
        if (classFreq > m_iMaxFrequency) {
            m_iMaxFrequency = classFreq;
        }
    }

    QWidget::update();
}

} // NAMESPACE

#endif // BAR_H
