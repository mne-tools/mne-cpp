//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     plot.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    MATLAB-style line plot for an Eigen vector, built on @ref Graph.
 *
 * Plot turns an @c Eigen::VectorXd into an axis-labelled 2-D line
 * drawing inside a @c QWidget, mirroring the basic behaviour of
 * MATLAB's @c plot(y) command. It is mainly used by demo applications
 * and quick-look windows where a single-trace preview is sufficient;
 * richer plots that need a real coordinate transform use @ref LinePlot
 * or the application-side Qt Charts backends instead.
 */

#ifndef PLOT_H
#define PLOT_H

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
 * @brief MATLAB-style line plot for an Eigen vector, derived from @ref Graph.
 *
 * Owns a list of @c QVector<QPointF> trace paths and supports a
 * @c m_bHoldOn flag for overlaying multiple curves. @c updateData()
 * replaces the visible curve in place.
 */
class DISPSHARED_EXPORT Plot : public Graph
{
    Q_OBJECT

public:
    typedef QSharedPointer<Plot> SPtr;            /**< Shared pointer type for Plot class. */
    typedef QSharedPointer<const Plot> ConstSPtr; /**< Const shared pointer type for Plot class. */

    //=========================================================================================================
    /**
     * Creates the plot.
     *
     * @param[in] parent     Parent QObject (optional).
     */
    explicit Plot(QWidget *parent = Q_NULLPTR);

    //=========================================================================================================
    /**
     * Creates the plot using a given double vector.
     *
     * @param[in] p_dVec     The double data vector.
     * @param[in] parent     Parent QObject (optional).
     */
    explicit Plot(Eigen::VectorXd &p_dVec,
                  QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destructs the Plot object
     */
    ~Plot();

    //=========================================================================================================
    /**
     * Initializes the Plot object
     */
    void init();

    //=========================================================================================================
    /**
     * Updates the plot using a given double vector without given X data.
     *
     * @param[in] p_dVec     The double data vector.
     */
    void updateData(Eigen::VectorXd &p_dVec);

protected:
    //=========================================================================================================
    /**
     * The reimplemented paintEvent
     *
     * @param[in] event    The event.
     */
    void paintEvent(QPaintEvent* event);

    QList<QVector<QPointF> > m_qListVecPointFPaths;     /**< List of point series. */

    bool    m_bHoldOn;          /**< If multiple plots. */
    double  m_dMinX;            /**< Minimal X value. */
    double  m_dMaxX;            /**< Maximal X value. */
    double  m_dMinY;            /**< Minimal Y value. */
    double  m_dMaxY;            /**< Maximal Y value. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE

#endif // PLOT_H
