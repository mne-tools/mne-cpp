//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     spline.h
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
 * @brief    Histogram widget with a cubic spline overlay and click-driven threshold markers.
 *
 * Spline draws the same class-limit / frequency data as @ref mne::viz::qt::Bar but
 * overlays a smoothed cubic-spline curve that follows the bin midpoints
 * and offers an interactive workflow: mouse clicks drop three vertical
 * threshold lines (left / middle / right) and emit signals carrying
 * their data-space positions. This is used by the histogram-based
 * artefact / noise-distribution dialogs to let the user pick lower,
 * centre and upper cut-off values without typing numbers.
 */

#ifndef SPLINE_H
#define SPLINE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_qt_global.h"
#include <mne/viz/figures/spline_plot.h>

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
// STL INCLUDES
//=============================================================================================================

#include <utility>
#include <vector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QMouseEvent;

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
 * @brief Histogram QWidget with cubic-spline overlay and interactive threshold markers.
 *
 * Click-drag drops left / middle / right threshold lines that are
 * emitted as data-space coordinates; otherwise the API mirrors
 * @ref Bar (@c setData / @c updatePlot templates).
 *
 * @snippet ex_disp_plots/main.cpp spline_usage
 */
class VIZQTSHARED_EXPORT Spline : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * The constructor for Spline
     * @param[in] parent    sets the behaviour of Spline as an object, defaults to no parent QWidget.
     * @param[in] title     title drawn above the histogram, defaults to "Spline Histogram".
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
    void mousePressEvent(QMouseEvent* event) override;

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
     * setColorMap sets the color mapping on the histogram background
     *
     * @param[in] colorMap  qstring of the color gradient from user input.
     */
    void setColorMap(const QString& colorMap);

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
     * Paints the spline chart.
     *
     * @param[in] event  The paint event.
     */
    void paintEvent(QPaintEvent* event) override;

    //=========================================================================================================
    /**
     * Converts data X coordinate to pixel X coordinate.
     *
     * @param[in] dataX  X value in data (axis) units.
     *
     * @return Horizontal widget pixel position; the left margin if the X axis range is empty.
     */
    double dataToPixelX(double dataX) const;

    //=========================================================================================================
    /**
     * Converts data Y coordinate to pixel Y coordinate.
     *
     * @param[in] dataY  Y value (frequency count) in data units.
     *
     * @return Vertical widget pixel position, scaled against the maximum frequency.
     */
    double dataToPixelY(double dataY) const;

    //=========================================================================================================
    /**
     * Converts pixel X coordinate to data X coordinate.
     *
     * @param[in] pixelX  Horizontal widget pixel position.
     *
     * @return X value in data units; the axis minimum if the plot area has no width.
     */
    double pixelToDataX(double pixelX) const;

    //=========================================================================================================
    /**
     * Returns the left margin in pixels.
     *
     * @return Left plot margin in pixels (60).
     */
    int leftMargin() const
    {
        return static_cast<int>(viz::SplinePlot::kLeftMargin);
    }

    //=========================================================================================================
    /**
     * Returns the right margin in pixels.
     *
     * @return Right plot margin in pixels (20).
     */
    int rightMargin() const
    {
        return static_cast<int>(viz::SplinePlot::kRightMargin);
    }

    //=========================================================================================================
    /**
     * Returns the top margin in pixels.
     *
     * @return Top plot margin in pixels (30).
     */
    int topMargin() const
    {
        return static_cast<int>(viz::SplinePlot::kTopMargin);
    }

    //=========================================================================================================
    /**
     * Returns the bottom margin in pixels.
     *
     * @return Bottom plot margin in pixels (60).
     */
    int bottomMargin() const
    {
        return static_cast<int>(viz::SplinePlot::kBottomMargin);
    }

    viz::SplinePlot m_plot;      /**< The plot, drawn through the QPainter backend. */
    QVector3D m_vecReturnVector; /**< Cached return value for getThreshold. */

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

template<typename T>
void Spline::setData(const Eigen::Matrix<T, Eigen::Dynamic, 1>& matClassLimitData,
                     const Eigen::Matrix<int, Eigen::Dynamic, 1>& matClassFrequencyData)
{
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> matrixName(matClassLimitData.rows(), 1);
    matrixName.col(0) = matClassLimitData;
    this->updatePlot(matrixName, matClassFrequencyData);
}

//=========================================================================================================

template<typename T>
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
    int iClassAmount = std::min(static_cast<int>(matClassFrequencyData.rows()),
                                static_cast<int>(matClassLimitData.rows()) - 1);
    if (iClassAmount < 1)
        return;
    std::vector<viz::Point> classMarks;
    for (int ir = 0; ir < iClassAmount; ++ir) {
        const double classMark = (static_cast<double>(matClassLimitData(ir)) + static_cast<double>(matClassLimitData(ir + 1))) / 2.0;
        classMarks.push_back({classMark, static_cast<double>(matClassFrequencyData(ir))});
    }
    m_plot.set_classes(std::move(classMarks), static_cast<double>(matClassLimitData(0)), static_cast<double>(matClassLimitData(iClassAmount)));

    QWidget::update();
}

} // NAMESPACE

#endif // SPLINE_H
