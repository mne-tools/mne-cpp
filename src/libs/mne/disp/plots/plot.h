//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     plot.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     June 2013
 * @brief    MATLAB-style line plot for an Eigen vector, built on @ref mne::disp::Graph.
 *
 * Plot turns an @c Eigen::VectorXd into an axis-labelled 2-D line
 * drawing inside a @c QWidget, mirroring the basic behaviour of
 * MATLAB's @c plot(y) command. It is mainly used by demo applications
 * and quick-look windows where a single-trace preview is sufficient;
 * richer plots that need a real coordinate transform use @ref mne::disp::LinePlot
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
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE mne::disp
//=============================================================================================================

namespace mne::disp
{

//=============================================================================================================
// mne::disp FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * @brief MATLAB-style line plot for an Eigen vector, derived from @ref Graph.
 *
 * Owns a list of @c std::vector<QPointF> trace paths and supports a
 * @c m_bHoldOn flag for overlaying multiple curves. @c updateData()
 * replaces the visible curve in place.
 *
 * @snippet ex_disp_plots/main.cpp plot_usage
 */
class DISPSHARED_EXPORT Plot : public Graph
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Creates the plot.
     *
     * @param[in] parent     Parent QObject (optional).
     */
    explicit Plot(QWidget* parent = nullptr);

    //=========================================================================================================
    /**
     * Creates the plot using a given double vector.
     *
     * @param[in] p_dVec     The double data vector.
     * @param[in] parent     Parent QObject (optional).
     */
    explicit Plot(Eigen::VectorXd& p_dVec,
                  QWidget* parent = 0);

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
    void updateData(Eigen::VectorXd& p_dVec);

protected:
    //=========================================================================================================
    /**
     * The reimplemented paintEvent
     *
     * @param[in] event    The event.
     */
    void paintEvent(QPaintEvent* event);

    std::vector<std::vector<QPointF>> m_qListVecPointFPaths; /**< List of point series. */

    bool m_bHoldOn; /**< If multiple plots. */
    double m_dMinX; /**< Minimal X value. */
    double m_dMaxX; /**< Maximal X value. */
    double m_dMinY; /**< Minimal Y value. */
    double m_dMaxY; /**< Maximal Y value. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE

#endif // PLOT_H
