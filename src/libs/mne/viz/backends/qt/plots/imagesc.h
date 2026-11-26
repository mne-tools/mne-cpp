//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     imagesc.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     June 2013
 * @brief    MATLAB-style false-colour heat-map widget for Eigen matrices.
 *
 * ImageSc visualises an @c Eigen::MatrixXd / @c MatrixXf / @c MatrixXi as
 * a 2-D image, mapping the data range linearly into a selectable
 * colour map (Jet, Hot, Bone, ...) through @ref mne::viz::qt::ColorMap. It inherits
 * the title / axis-label scaffolding from @ref mne::viz::qt::Graph and is the canvas
 * of choice for sensor-by-sample heat maps, covariance matrices and
 * any other rectangular numeric field that benefits from a glance-able
 * false-colour rendering. Calling @ref mne::viz::qt::ImageSc::updateData "ImageSc::updateData" repaints the
 * scaled view with new contents.
 */

#ifndef IMAGESC_H
#define IMAGESC_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_qt_global.h"
#include "graph.h"
#include <mne/viz/figures/image_plot.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

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
 * @brief False-colour heat-map QWidget visualising an Eigen matrix with selectable colour map.
 *
 * Accepts @c MatrixXd, @c MatrixXf and @c MatrixXi inputs and draws them
 * as an mne::viz::ImagePlot, which renders without Qt as well.
 *
 * @snippet ex_disp_plots/main.cpp image_sc_usage
 */
class VIZQTSHARED_EXPORT ImageSc : public Graph
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Creates the scaled image view.
     *
     * @param[in] parent     Parent QObject (optional).
     */
    explicit ImageSc(QWidget* parent = 0);

    //=========================================================================================================
    /**
     * Creates the scaled image view with a given double matrix.
     *
     * @param[in] p_dMat     The double data matrix.
     * @param[in] parent     Parent QObject (optional).
     */
    explicit ImageSc(Eigen::MatrixXd& p_dMat,
                     QWidget* parent = 0);

    //=========================================================================================================
    /**
     * Creates the scaled image view with a given float matrix.
     *
     * @param[in] p_fMat     The float data matrix.
     * @param[in] parent     Parent QObject (optional).
     */
    explicit ImageSc(Eigen::MatrixXf& p_fMat,
                     QWidget* parent = 0);

    //=========================================================================================================
    /**
     * Creates the scaled image view with a given integer matrix.
     *
     * @param[in] p_iMat     The integer data matrix.
     * @param[in] parent     Parent QObject (optional).
     */
    explicit ImageSc(Eigen::MatrixXi& p_iMat,
                     QWidget* parent = 0);

    //=========================================================================================================
    /**
     * Destructs the ImageSc object
     */
    ~ImageSc();

    //=========================================================================================================
    /**
     * Initializes the ImageSc object
     */
    void init();

    //=========================================================================================================
    /**
     * Updates the scaled image view with a given double matrix.
     *
     * @param[in] p_dMat     The double data matrix.
     */
    void updateData(Eigen::MatrixXd& p_dMat);

    //=========================================================================================================
    /**
     * Updates the scaled image view with a given float matrix.
     *
     * @param[in] p_fMat     The float data matrix.
     */
    void updateData(Eigen::MatrixXf& p_fMat);
    //=========================================================================================================
    /**
     * Updates the scaled image view with a given integer matrix.
     *
     * @param[in] p_iMat     The integer data matrix.
     */
    void updateData(Eigen::MatrixXi& p_iMat);

    //=========================================================================================================
    /**
     * Sets the color map to use, e.g. "Jet", "Hot", "Bone"
     *
     * @param[in] p_sColorMap    The colormap to use.
     */
    void setColorMap(const QString& p_sColorMap);

    //=========================================================================================================
    /**
     * Labels the axes in data units instead of column and row indices, as matplotlib's @c imshow(extent=...).
     *
     * @param[in] left       X value of the first column.
     * @param[in] right      X value of the last column.
     * @param[in] bottom     Y value of the last row, drawn at the bottom.
     * @param[in] top        Y value of the first row, drawn at the top.
     */
    void setExtent(double left, double right, double bottom, double top);

    //=========================================================================================================
    /**
     * Maps @p min and @p max to the ends of the colour map instead of the data minimum and maximum; values
     * outside are clipped. A range symmetric around zero centres diverging maps such as "RedBlue".
     *
     * @param[in] min    Value drawn with the low end of the colour map.
     * @param[in] max    Value drawn with the high end of the colour map.
     */
    void setColorRange(double min, double max);

    //=========================================================================================================
    /**
     * Names the quantity the colour bar shows, e.g. "Power (normalized)".
     *
     * @param[in] label  The colour bar label; empty hides it.
     */
    void setColorbarLabel(const QString& label);

protected:
    //=========================================================================================================
    /**
     * The reimplemented paintEvent
     *
     * @param[in] event    The event.
     */
    void paintEvent(QPaintEvent* event) override;

    viz::ImagePlot m_plot; /**< The plot, drawn through the QPainter backend. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE

#endif // IMAGESC_H
