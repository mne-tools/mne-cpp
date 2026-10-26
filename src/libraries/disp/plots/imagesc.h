//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     imagesc.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    MATLAB-style false-colour heat-map widget for Eigen matrices.
 *
 * ImageSc visualises an @c Eigen::MatrixXd / @c MatrixXf / @c MatrixXi as
 * a 2-D image, mapping the data range linearly into a selectable
 * colour map (Jet, Hot, Bone, ...) through @ref DISPLIB::ColorMap. It inherits
 * the title / axis-label scaffolding from @ref DISPLIB::Graph and is the canvas
 * of choice for sensor-by-sample heat maps, covariance matrices and
 * any other rectangular numeric field that benefits from a glance-able
 * false-colour rendering. Calling @ref DISPLIB::ImageSc::updateData "ImageSc::updateData" repaints the
 * scaled view with new contents.
 */

#ifndef IMAGESC_H
#define IMAGESC_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "graph.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPixmap>

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
 * @brief False-colour heat-map QWidget visualising an Eigen matrix with selectable colour map.
 *
 * Accepts @c MatrixXd, @c MatrixXf and @c MatrixXi inputs; @c init()
 * rebuilds the cached @c QImage from the current matrix and the
 * active colour map, and @c updateData() / @c paintEvent() keep the
 * rendering in sync with the data.
 *
 * @snippet ex_disp_plots/main.cpp image_sc_usage
 */
class DISPSHARED_EXPORT ImageSc : public Graph
{
    Q_OBJECT

public:
    typedef QSharedPointer<ImageSc> SPtr;            /**< Shared pointer type for ImageSc class. */
    typedef QSharedPointer<const ImageSc> ConstSPtr; /**< Const shared pointer type for ImageSc class. */

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
     * Maps the data to [0, 1] by the colour range, clipping values outside it.
     */
    void normalize();

    //=========================================================================================================
    /**
     * Updates data and colorbar pixmap
     */
    void updateMaps();

    //=========================================================================================================
    /**
     * The reimplemented paintEvent
     *
     * @param[in] event    The event.
     */
    void paintEvent(QPaintEvent* event) override;

    QPixmap m_pixmapData;     /**< data pixmap. */
    QPixmap m_pixmapColorbar; /**< colorbar pixmap. */

    QString m_sColorMap; /**< The colorbar. */

    Eigen::MatrixXd m_matData;         /**< The data as given. */
    Eigen::MatrixXd m_matCentNormData; /**< Data mapped to [0, 1] by the colour range. */

    double m_dMinValue;    /**< Value at the low end of the colour map. */
    double m_dMaxValue;    /**< Value at the high end of the colour map. */
    bool m_bColorRangeSet; /**< Whether setColorRange() fixed the colour range. */

    double m_dLeft;    /**< X value of the first column. */
    double m_dRight;   /**< X value of the last column. */
    double m_dBottom;  /**< Y value of the last row. */
    double m_dTop;     /**< Y value of the first row. */
    bool m_bExtentSet; /**< Whether setExtent() gave data units. */

    bool m_bColorbar;            /**< If colorbar is visible. */
    QString m_sColorbarLabel;    /**< Quantity shown by the colour bar. */
    qint32 m_iColorbarWidth;     /**< Colorbar width. */
    qint32 m_iColorbarGradSteps; /**< Gradient steps of the colorbar. */

    QRgb (*pColorMapper)(double, const QString&); /**< Function pointer to current colormap. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE

#endif // IMAGESC_H
