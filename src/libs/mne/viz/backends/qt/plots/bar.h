//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     bar.h
 * @author   Ricky Tjen <ricky270@student.sgu.ac.id>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>
 * @since    0.1.0
 * @date     April 2016
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

#include "../viz_qt_global.h"
#include <mne/viz/figures/bar_chart.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <format>
#include <string>
#include <utility>
#include <vector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE mne::viz::qt
//=============================================================================================================

namespace mne::viz::qt
{

//=============================================================================================================
// mne::viz::qt FORWARD DECLARATIONS
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
class VIZQTSHARED_EXPORT Bar : public QWidget
{
    Q_OBJECT

public:
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
    viz::BarChart m_chart; /**< The chart, drawn through the QPainter backend. */
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
    std::vector<std::string> categories;
    std::vector<int> counts;
    for (int kr = 0; kr < iClassAmount; ++kr) {
        categories.push_back(std::format("{:.{}g} to {:.{}g}", static_cast<double>(matClassLimitData(kr)), iPrecisionValue, static_cast<double>(matClassLimitData(kr + 1)), iPrecisionValue));
        counts.push_back(matClassFrequencyData(kr));
    }
    m_chart.set_bars(std::move(categories), std::move(counts));

    QWidget::update();
}

} // NAMESPACE

#endif // BAR_H
