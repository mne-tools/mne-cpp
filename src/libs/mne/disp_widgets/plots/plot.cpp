//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     plot.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     June 2013
 * @brief    Implementation of the Plot vector-plot widget (auto-range and polyline painting).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "plot.h"
#include <mne/disp/plots/helpers/scenepainter.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPainter>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::disp;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Plot::Plot(QWidget* parent)
: Graph(parent)
{
    init();
}

//=============================================================================================================

Plot::Plot(VectorXd& p_dVec, QWidget* parent)
: Graph(parent)
{
    init();
    updateData(p_dVec);
}

//=============================================================================================================

Plot::~Plot()
{
}

//=============================================================================================================

void Plot::init()
{
    //Parent init
    Graph::init();

    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);

    m_plot = scene::VectorPlot();
}

//=============================================================================================================

void Plot::updateData(VectorXd& p_dVec)
{
    if (p_dVec.size() > 0) {
        // Replace the curves; title and axis labels stay
        m_plot.set_data(std::vector<double>(p_dVec.data(), p_dVec.data() + p_dVec.size()));
        update();
    }
}

//=============================================================================================================

void Plot::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    m_plot.set_title(m_sTitle.toStdString());
    m_plot.set_x_label(m_sXLabel.toStdString());
    m_plot.set_y_label(m_sYLabel.toStdString());
    QPainter painter(this);
    const QtTextMetrics metrics(m_qFontAxes);
    scene::Canvas canvas(m_qSizeWidget.width(), m_qSizeWidget.height());
    m_plot.draw(canvas, metrics);
    paintScene(painter, canvas.display_list(), m_qFontAxes);
}
