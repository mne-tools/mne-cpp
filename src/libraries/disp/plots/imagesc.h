//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file imagesc.h
 * @since 2022
 * @date  October 2022
 * @brief MATLAB-style false-colour heat-map widget for Eigen matrices.
 *
 * ImageSc visualises an @c Eigen::MatrixXd / @c MatrixXf / @c MatrixXi as
 * a 2-D image, mapping the data range linearly into a selectable
 * colour map (Jet, Hot, Bone, ...) through @ref ColorMap. It inherits
 * the title / axis-label scaffolding from @ref Graph and is the canvas
 * of choice for sensor-by-sample heat maps, covariance matrices and
 * any other rectangular numeric field that benefits from a glance-able
 * false-colour rendering. Calling @ref ImageSc::updateData repaints the
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
    explicit ImageSc(QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Creates the scaled image view with a given double matrix.
     *
     * @param[in] p_dMat     The double data matrix.
     * @param[in] parent     Parent QObject (optional).
     */
    explicit ImageSc(Eigen::MatrixXd &p_dMat,
                     QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Creates the scaled image view with a given float matrix.
     *
     * @param[in] p_fMat     The float data matrix.
     * @param[in] parent     Parent QObject (optional).
     */
    explicit ImageSc(Eigen::MatrixXf &p_fMat,
                     QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Creates the scaled image view with a given integer matrix.
     *
     * @param[in] p_iMat     The integer data matrix.
     * @param[in] parent     Parent QObject (optional).
     */
    explicit ImageSc(Eigen::MatrixXi &p_iMat,
                     QWidget *parent = 0);

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
    void updateData(Eigen::MatrixXd &p_dMat);

    //=========================================================================================================
    /**
     * Updates the scaled image view with a given float matrix.
     *
     * @param[in] p_fMat     The float data matrix.
     */
    void updateData(Eigen::MatrixXf &p_fMat);
    //=========================================================================================================
    /**
     * Updates the scaled image view with a given integer matrix.
     *
     * @param[in] p_dMat     The integer data matrix.
     */
    void updateData(Eigen::MatrixXi &p_iMat);

    //=========================================================================================================
    /**
     * Sets the color map to use, e.g. "Jet", "Hot", "Bone"
     *
     * @param[in] p_sColorMap    The colormap to use.
     */
    void setColorMap(const QString &p_sColorMap);

protected:
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
    void paintEvent(QPaintEvent* event);

    QPixmap*            m_pPixmapData;              /**< data pixmap. */
    QPixmap*            m_pPixmapColorbar;          /**< colorbar pixmap. */

    QString             m_sColorMap;                /**< The colorbar. */

    Eigen::MatrixXd     m_matCentNormData;          /**< centralized and normalized data. */

    double              m_dMinValue;                /**< Minimal data value. */
    double              m_dMaxValue;                /**< Maximal data value. */

    bool                m_bColorbar;                /**< If colorbar is visible. */
    QVector<double>     m_qVecScaleValues;          /**< Scale values. */
    qint32              m_iColorbarWidth;           /**< Colorbar width. */
    qint32              m_iColorbarSteps;           /**< Number of colorbar vaues to display. */
    qint32              m_iColorbarGradSteps;       /**< Gradient steps of the colorbar. */
    QFont               m_qFontColorbar;            /**< Colorbar font. */
    QPen                m_qPenColorbar;             /**< Colorbar pen. */

    QRgb                (*pColorMapper)(double, const QString&);    /**< Function pointer to current colormap. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE

#endif // IMAGESC_H
